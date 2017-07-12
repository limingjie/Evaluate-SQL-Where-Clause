#include <iostream>  // cout
#include <map>       // map
#include <memory>    // shared_ptr
#include <stdexcept> // stoi, stod
#include <string>    // string, stoi, stod
#include <vector>    // vector

typedef std::map<std::string, int> header_t;
typedef std::vector<std::string>   row_t;
typedef std::vector<row_t>         table_t;
typedef int                        operator_t;

class Operator
{
public:
    static const operator_t EQ  = 0x00;
    static const operator_t NE  = 0x01;
    static const operator_t LT  = 0x02;
    static const operator_t LE  = 0x03;
    static const operator_t GT  = 0x04;
    static const operator_t GE  = 0x05;
    static const operator_t AND = 0x07;
    static const operator_t OR  = 0x08;

    static const std::string toString(operator_t op)
    {
        static std::string ops[8] = {"=", "!=", "<", "<=", ">", ">=", "AND", "OR"};

        return ops[op];
    }
};

// Base Condition class, to make sure all types of conditions can be invoked using the same base type.
class ConditionBase
{
public:
    // A table class should be defined to encapsulate table header and table rows,
    // then the function parameter could be (table, row_index).
    virtual bool eval(header_t& header, row_t& row) = 0;

    virtual ~ConditionBase()
    {
        // nothing here, but required by polymorphism.
    }
};

// Subclass for any type of conditions.
template <typename T>
class Condition: public ConditionBase
{
private:
    std::string column;
    operator_t op;
    T value;

    bool getColumnValue(header_t& header, row_t& row, T& val)
    {
        val = row[header[column]];
        return true;
    }

public:
    // construct a new condition. i.e. name = "John Doe"
    Condition(const std::string& column, operator_t op, const T& value)
    {
        this->column = column;
        this->op     = op;
        this->value  = value;
    }

    bool eval(header_t& header, row_t& row)
    {
        bool result;
        T val;
        if (getColumnValue(header, row, val))
        {
            switch(op)
            {
                case Operator::EQ: result = (val == value); break;
                case Operator::NE: result = (val != value); break;
                case Operator::LT: result = (val <  value); break;
                case Operator::LE: result = (val <= value); break;
                case Operator::GT: result = (val >  value); break;
                case Operator::GE: result = (val >= value); break;
                default:           result = false;          break;
            }
        }
        else // conversion error
        {
            result = false;
        }

        // Debug
        // std::cout << "#Debug: "
        //           << column << ' ' << Operator::toString(op) << ' ' << value
        //           << " -> " << column << " = " << val << " -> "
        //           << (result ? "true" : "false") << std::endl;

        return result;
    }
};

// Handle integer
template <>
bool Condition<int>::getColumnValue(header_t& header, row_t& row, int &val)
{
    try
    {
        val = std::stoi(row[header[column]]);
    }
    catch(const std::invalid_argument& e)
    {
        return false;
    }
    catch(const std::out_of_range& e)
    {
        return false;
    }

    return true;
}

// Handle floating point number
template <>
bool Condition<float>::getColumnValue(header_t& header, row_t& row, float &val)
{
    try
    {
        val = (float)std::stod(row[header[column]]);
    }
    catch(const std::invalid_argument& e)
    {
        return false;
    }
    catch(const std::out_of_range& e)
    {
        return false;
    }

    return true;
}

// Where Clause
class Where
{
private:
    std::vector<ConditionBase*> conditions;  // all conditions in the clause
    std::vector<operator_t>     operators;   // all operators in the clause

public:

    ~Where()
    {
        for (size_t i = 0; i < conditions.size(); i++)
        {
            delete conditions[i];
        }
    }

    Where* AddCondition(ConditionBase* c)
    {
        conditions.push_back(c);
        return this;
    }

    Where* AddOperator(operator_t op)
    {
        operators.push_back(op);
        return this;
    }

    // A table class should be defined to encapsulate table header and table rows,
    // then the function parameter could be (table, row_index, op_index).
    bool eval(header_t& header, row_t& row)
    {
        bool result = conditions[0]->eval(header, row);

        size_t op_index = 0;
        while(op_index < operators.size())
        {
            if (operators[op_index] == Operator::AND)
            {
                op_index++;

                // If one operand of AND is false, skip :)
                if (result)
                {
                    result = conditions[op_index]->eval(header, row);
                }
                else
                {
                    continue;
                }
            }
            else // OR
            {
                op_index++;

                // If one operand of OR is true, stop here :)
                if (result)
                {
                    return true;
                }
                else
                {
                    result = conditions[op_index]->eval(header, row);
                    continue;
                }
            }
        }

        return result;
    }
};

int main()
{
    header_t header = {{"name", 0}, {"age", 1}, {"gender", 2}, {"score", 3}, {"company", 4}};
    table_t table = {
        {"John Doe",   "20", "male",   "110.5", "IBX"      },
        {"Jenny Ho",   "21", "female", "100",   "Huawei"   },
        {"Bill Gates", "61", "male",   "101",   "Microsoft"},
        {"Paul Allen", "64", "male",   "102",   "Microsoft"},
        {"Jane Doe",   "32", "female", "199",   "Microsoft"}
    };

    // WHERE name != "Bill Gates" AND age > 60 OR gender = "female" AND score <= 100 OR company = "IBX"
    std::shared_ptr<Where> w(new Where());
    w->AddCondition(new Condition<std::string>("name", Operator::NE, "Bill Gates"))
     ->AddOperator(Operator::AND)
     ->AddCondition(new Condition<int>("age", Operator::GT, 30))
     ->AddOperator(Operator::OR)
     ->AddCondition(new Condition<std::string>("gender", Operator::EQ, "female"))
     ->AddOperator(Operator::AND)
     ->AddCondition(new Condition<float>("score", Operator::LE, 100))
     ->AddOperator(Operator::OR)
     ->AddCondition(new Condition<std::string>("company", Operator::EQ, "IBX"));

    std::cout << "name\t\tage\tgender\tscore\tcompany\n"
              << "---------+---------+---------+---------+---------+\n";
    for (size_t i = 0; i < table.size(); i++)
    {
        if (w->eval(header, table[i]))
        {
            for (size_t j = 0; j < table[i].size(); j++)
            {
                std::cout << table[i][j] << '\t';
            }
            std::cout << std::endl;
        }
    }

    return 0;
}
