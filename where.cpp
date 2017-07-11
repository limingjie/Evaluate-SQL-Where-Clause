#include <iostream>
#include <string>
#include <vector>
#include <map>

typedef std::map<std::string, int> header_t;
typedef std::vector<std::string> row_t;
typedef std::vector<row_t> table_t;
typedef int operator_t;

class ConditionBase
{
public:
    static const operator_t EQ = 0;
    static const operator_t NE = 1;
    static const operator_t LT = 2;
    static const operator_t LE = 3;
    static const operator_t GT = 4;
    static const operator_t GE = 5;

    virtual bool eval(header_t &header, row_t &row) = 0;
    virtual void print() = 0;
};

template <typename T>
class Condition: public ConditionBase
{
private:
    std::string column;
    operator_t op;
    T value;
public:
    Condition(std::string column, operator_t op, T value)
    {
        this->column = column;
        this->op = op;
        this->value = value;
    }

    bool eval(header_t &header, row_t &row);

    void print()
    {
        std::string ops[6] = {"=", "!=", "<", "<=", ">", ">="};
        std::cout << column << ' ' << ops[op] << ' ' << value << std::endl;
    }
};

template <>
bool Condition<int>::eval(header_t &header, row_t &row)
{
    int val = std::stoi(row[header[column]]);
    switch(op)
    {
        case EQ: return val == value;
        case NE: return val != value;
        case LT: return val <  value;
        case LE: return val <= value;
        case GT: return val >  value;
        case GE: return val >= value;
        default: return false;
    }
}

template <>
bool Condition<float>::eval(header_t &header, row_t &row)
{
    float val = (float)std::stod(row[header[column]]);
    switch(op)
    {
        case EQ: return val == value;
        case NE: return val != value;
        case LT: return val <  value;
        case LE: return val <= value;
        case GT: return val >  value;
        case GE: return val >= value;
        default: return false;
    }
}

template <>
bool Condition<std::string>::eval(header_t &header, row_t &row)
{
    std::string val = row[header[column]];
    switch(op)
    {
        case EQ: return val == value;
        case NE: return val != value;
        case LT: return val <  value;
        case LE: return val <= value;
        case GT: return val >  value;
        case GE: return val >= value;
        default: return false;
    }
}

class Where
{
private:
    std::vector<ConditionBase *> conditions;
    std::vector<operator_t> operators;
public:
    static const operator_t AND = 0;
    static const operator_t OR  = 1;

    Where *AddCondition(ConditionBase *c)
    {
        conditions.push_back(c);
        return this;
    }

    Where *AddOperator(operator_t op)
    {
        operators.push_back(op);
        return this;
    }

    bool eval(header_t &header, row_t &row, int index = 0)
    {
        bool result = conditions[index]->eval(header, row);

        // debug
        // conditions[index]->print();
        // std::cout << " -> " << (result ? "true" : "false") << std::endl;

        while(index < operators.size())
        {
            if (operators[index] == AND)
            {
                index++;
                result = result && conditions[index]->eval(header, row);

                // debug
                // conditions[index]->print();
                // std::cout << " -> " << (conditions[index]->eval(header, row) ? "true" : "false") << std::endl;
            }
            else // OR
            {
                index++;
                result = result || eval(header, row, index);
                break; // break here, thus this function can be written as non-recursive.
            }
        }

        return result;
    }
};

int main()
{
    header_t header = {{"name", 0}, {"age", 1}, {"gender", 2}, {"score", 3}, {"company", 4}};
    table_t table = {
        {"John Doe", "20", "male", "110.5", "IBX"},
        {"Jenny Ho", "21", "female", "100", "Huawei"},
        {"Bill Gates", "61", "male", "101", "Microsoft"},
        {"Paul Allen", "64", "male", "102", "Microsoft"},
        {"Jan Doe", "32", "female", "199", "Microsoft"}
    };

    // WHERE name != "Bill Gates" AND age > 60 OR gender = "female" AND score <= 100 OR company = "IBX"
    Where *w = new Where();
    w->AddCondition(new Condition<std::string>("name", ConditionBase::NE, "Bill Gates"))
     ->AddOperator(Where::AND)
     ->AddCondition(new Condition<int>("age", ConditionBase::GT, 60))
     ->AddOperator(Where::OR)
     ->AddCondition(new Condition<std::string>("gender", ConditionBase::EQ, "female"))
     ->AddOperator(Where::AND)
     ->AddCondition(new Condition<float>("score", ConditionBase::LE, 100))
     ->AddOperator(Where::OR)
     ->AddCondition(new Condition<std::string>("company", ConditionBase::EQ, "IBX"));

    for (int i = 0; i < table.size(); i++)
    {
        if (w->eval(header, table[i]))
        {
            for (int j = 0; j < table[i].size(); j++)
            {
                std::cout << table[i][j] << '\t';
            }
            std::cout << std::endl;
        }
    }

    return 0;
}
