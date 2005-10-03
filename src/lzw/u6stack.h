#include <vector>

using std::vector;

class U6Decode::Stack {
public:
    bool is_empty() {
        return stack.size() == 0;
    }

    bool is_full() {
        return stack.size() == stack_size;
    }

    void push(unsigned char element) {
        if (!is_full()) {
            stack.push_back(element);
        }
    }

    unsigned char pop() {
        unsigned char element;
      
        if (!is_empty()) {
            element = stack.back();
            stack.pop_back();
        }
        else {
            element = 0;
        }
        return element;
    }

    unsigned char gettop() {
        if (!is_empty())
            return stack.back();
        else
            return 0;
    }

private:
    static const unsigned int stack_size = 10000;

    vector<unsigned char> stack;
};


