
// build this using the following command: g++ --std=c++11 -O0 -g3 -o singleton singleton_test.cc

#include <iostream>
#include <memory> // For unique_ptr.

using namespace std;

class Singleton {
public:
    static Singleton *get_instance_ptr( )
    {
        return &get_instance();
    }

    static Singleton &get_instance( )
    {
        if (instance == nullptr) {
            static std::once_flag d_euc_init_once;
            std::call_once(d_euc_init_once, []() {
                cout << "Initializing instance.\n";
                instance.reset(new Singleton()); // Create new instance, assign to unique_ptr.
            });
#if 0
            // This is the same as the above, but omits the static std::once_flag.
            cout << "Creating instance.\n";
            instance.reset(new Singleton());
#endif
        }
        return *instance;
    }

    // Could/Should be replaced with the default destructor.
    ~Singleton( )
    {
        std::cout << "Destructor called.\n";
        if (instance == nullptr) {
            std::cout << "* Instance already deleted!\n";
        }
    }

    int get_calls() { calls += 1; return calls; }

private:
    int calls = 0;

    Singleton() = default; // Private constructor.
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

    // This version just uses delete to destroy the pointer.
    static std::unique_ptr<Singleton> instance;
};

std::unique_ptr<Singleton> Singleton::instance = nullptr;

int main( )
{
    Singleton &singleton = Singleton::get_instance();
    cout << "singleton: " << &singleton << endl;
    cout << "Calls: " << singleton.get_calls() << endl;

    Singleton &s2 = Singleton::get_instance();
    cout << "s2: " << &s2 << endl;
    cout << "Calls: " << s2.get_calls() << endl;

    Singleton &s3 = Singleton::get_instance();
    cout << "s3: " << &s3 << endl;
    cout << "Calls: " << s3.get_calls() << endl;

    Singleton *s4 = Singleton::get_instance_ptr();
    cout << "s4: " << s4 << endl;
    cout << "Calls: " << s4->get_calls() << endl;

    return 0;
}
