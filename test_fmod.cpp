#include <iostream>
#include <iomanip>
#include <cmath>
#include <array>
#include <bitset>
#include <vector>
#include <boost/functional/hash.hpp>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include <boost/predef.h>
#include <random>
#include <fstream>

double my_fmod (double x, double y);

using namespace std;

typedef union { uint32_t a[2]; uint64_t i; uint8_t b[8]; double x; } mynumber;

constexpr uint64_t random_numbers[] = {
        UINT64_C(0xA2C8B877585CF27B),
        UINT64_C(0xB500C85FF4024153),
        UINT64_C(0x7A99BA5628F8C1C1),
        UINT64_C(0x0D04E05E91147E21),
        UINT64_C(0x12099DE6BC6AFA62),
        UINT64_C(0x5ADF8FD793E00727),
        UINT64_C(0xF7002F54BBF0B21B),
        UINT64_C(0x4755AD393D09ED99),
        UINT64_C(0xCA10A5131AD12E4F),
        UINT64_C(0xC44A2E3FC98CC2DA)
};

class test_fmod_base {

public:
    struct test_result {
        std::size_t hash_output = UINT64_C(0x8370B2F63E16DFB9);
        std::size_t hash_input  = UINT64_C(0x4bA6C5A2EA5458A1);
        std::uint64_t total_calcs = 0;
        std::uint64_t sum_results = 0;
        std::uint64_t dt; //nanoseconds
        test_result() = default;
    };

    static
    std::string report(const test_result &r, int ind = 0) {
        std::string inds(ind, ' ');
        double time = (double(r.dt) * 1E-9) / (double(r.total_calcs) / 1E9);
        auto result =
                boost::format("%1%C++ HASH input values      : 0x%2$016X\n"
                              "%1%C++ HASH output values     : 0x%3$016X\n"
                              "%1%Total calculations         : %4%\n"
                              "%1%Time                       : %5$.3f (s for billion calls)\n"
                ) % inds % r.hash_input % r.hash_output %  r.total_calcs % time;
        return result.str();
    }
};

class test_fmod_main : public test_fmod_base {
private:
    static constexpr int bits_change = 14;
    static constexpr int bits_num = (1 << bits_change);

    vector<uint64_t> masksx, masksy;
    vector<int> fct_x, fct_y;

    mynumber create_num(uint32_t i, uint64_t mask) {
        mynumber result;
        result.i  = uint64_t(i) << (64 - bits_change);
        result.i |= mask & (UINT64_C(0xFFFFFFFFFFFFFFFF) >> bits_change);
        return result;
    }

    static vector<uint64_t> create_masks() {
        vector<uint64_t> result(cbegin(random_numbers), cend(random_numbers));
        for(int i = 0; i < 64 - bits_change; i++) {
            result.emplace_back( (UINT64_C(1) << i) );
            result.emplace_back( (UINT64_C(1) << i) + 1 );
        }
        return result;
    }

    static vector<int> create_factors() {
        vector<int> result(bits_num);
        iota(result.begin(), result.end(), 0);
        return result;
    }

    template<class C> static
    C reduce_size(const C &cont, int r_factor, int seed, int first_store) {
        C result = cont;
        if( r_factor > 1 ) {
            shuffle(result.begin() + first_store, result.end(), std::default_random_engine(seed));
            result.resize(min<int>(result.size(), result.size() / r_factor));
        }
        return result;
    }
public:

    void set_vals(int r_factor) {
        auto m = create_masks();
        auto f = create_factors();
        masksx = reduce_size(m, r_factor, 1, 2);
        masksy = reduce_size(m, r_factor, 2, 2);
        fct_x = reduce_size(f, r_factor, 3, 10);
        fct_y = reduce_size(f, r_factor, 4, 10);
    }

    template<typename F>
    test_result do_test(F f) {
        test_result result;
        boost::timer::cpu_timer at;
        at.start();
        for (int i : fct_x) {
            for (const auto &mi : masksx) {
                mynumber n1 = create_num(i, mi);
                boost::hash_combine(result.hash_input, boost::hash_value(n1.i));
                for (int j : fct_y) {
                    for (const auto &mj : masksy) {
                        mynumber n2 = create_num(j, mj);
                        mynumber r;
                        boost::hash_combine(result.hash_input, boost::hash_value(n2.i));
                        r.x = f(n1.x, n2.x);
                        result.sum_results += r.i;
                        boost::hash_combine(result.hash_output, boost::hash_value(r.i));
                        result.total_calcs++;
                    }
                }
            }
        }
        at.stop();
        result.dt = at.elapsed().wall;
        return result;
    }
};

class test_fmod_wrap : public test_fmod_base {
    double x_val_min;
    double x_val_max;
    uint64_t num_vals;
    double y_val;
public:
    test_fmod_wrap(double _x_val_min, double _x_val_max, uint64_t _num_vals, double _y_val) :
            x_val_min(_x_val_min), x_val_max(_x_val_max), num_vals(_num_vals), y_val(_y_val) {};
    template<typename F>
    test_result do_test(F f) {
        test_result result;
        boost::timer::cpu_timer at;
        at.start();
        for (uint64_t i = 0; i < num_vals; i++) {
            mynumber n1, r;
            n1.x = x_val_min + (x_val_max - x_val_min) * double(i) / double(num_vals);
            boost::hash_combine(result.hash_input, boost::hash_value(n1.i));
            r.x = f(n1.x, y_val);
            result.sum_results += r.i;
            boost::hash_combine(result.hash_output, boost::hash_value(r.i));
            result.total_calcs++;
        }
        at.stop();
        result.dt = at.elapsed().wall;
        return result;
    }
};

class test_fmod_extreme : public test_fmod_base {
    uint64_t num_vals;
public:
    test_fmod_extreme(uint64_t _num_vals) : num_vals(_num_vals) {};
    template<typename F>
    test_result do_test(F f) {
        test_result result;
        boost::timer::cpu_timer at;
        at.start();
        mynumber n1, n2, r;
        for (uint64_t i = 0; i < num_vals; i++) {
            n1.i =  0x7E3A47DC2F33B492UL + i; // huge number
            n2.i = 1 + i; // subnormal
            boost::hash_combine(result.hash_input, boost::hash_value(n1.i));
            boost::hash_combine(result.hash_input, boost::hash_value(n2.i));

            r.x = f(n1.x, n2.x);
            result.sum_results += r.i;
            boost::hash_combine(result.hash_output, boost::hash_value(r.i));
            result.total_calcs++;
        }
        at.stop();
        result.dt = at.elapsed().wall;
        return result;
    }
};
//fstream fs("chekv.dat", fstream::out);

inline
double dummy_f(double x, double y) {
    return x + y;
}

/*inline
double fmod_libm(double x, double y) {
    mynumber nx, nr;
    nx.x = x;
    nr.x = fmod(x, y);
    //fs << hex << nx.x << " " << nr.x << " " << nx.i << " " << nr.i << endl;
    //return fmod(x, y);
    return nr.x;
}*/

inline
double fmod_libm(double x, double y) {
    return fmod(x, y);
}

inline
double fmod_check(double x, double y) {
    /*if( !isfinite(x) || !isfinite(y) )
      return 0.0;
    if( abs(x) < 1E-280 || abs(y) < 1E-280 )
      return 0.0;*/
    mynumber xa, ya, ni, nn;
    xa.x = x;
    ya.x = y;
    ni.x = fmod(x, y);
    nn.x = my_fmod(x, y);
    if( nn.i != ni.i )
      cout << hex << ni.x << " " << nn.x << dec << endl;
    return ni.x;
}

int main() {
    {
        std::size_t control_hash = boost::hash_value(200);
        boost::hash_combine(control_hash, boost::hash_value(UINT64_MAX - 1000));
        cout << hex << "Test hash: 0x" << control_hash << endl
                    << "Equal to : 0x64061da93a405636" << dec << endl;
    }

    uint64_t num_vals = 6864400;
    {
        test_fmod_main tm;
        tm.set_vals(25); // Reduce factor default 25

        cout << "Main test: " << endl;

        auto r = tm.do_test(dummy_f);
        cout << "  Dummy function   :" << endl << test_fmod_base::report(r, 4) << endl;
        num_vals = r.total_calcs;

        r = tm.do_test(fmod_libm);
        cout << "  Internal function:" << endl << test_fmod_base::report(r, 4) << endl;

        r = tm.do_test(my_fmod);
        cout << "  New function     :" << endl << test_fmod_base::report(r, 4) << endl;
    }

    {
        test_fmod_wrap t_pi(-4 * M_PI, 4 * M_PI, 10 * num_vals, 2 * M_PI);

        cout << "Test wrap 2 * pi: " << endl;
        auto r = t_pi.do_test(dummy_f);
        cout << "  Dummy function   :" << endl << test_fmod_base::report(r, 4) << endl;

        r = t_pi.do_test(fmod_libm);
        cout << "  Internal function:" << endl << test_fmod_base::report(r, 4) << endl;

        r = t_pi.do_test(my_fmod);
        cout << "  New function     :" << endl << test_fmod_base::report(r, 4) << endl;
    }

    {
        test_fmod_wrap t_factor(-1E6, 1E6, 10 * num_vals, 1.0);

        cout << "Fraction part: " << endl;
        auto r = t_factor.do_test(dummy_f);
        cout << "  Dummy function   :" << endl << test_fmod_base::report(r, 4) << endl;

        r = t_factor.do_test(fmod_libm);
        cout << "  Internal function:" << endl << test_fmod_base::report(r, 4) << endl;

        r = t_factor.do_test(my_fmod);
        cout << "  New function     :" << endl << test_fmod_base::report(r, 4) << endl;
    }

    {
        test_fmod_extreme t_extr(num_vals);

        cout << "fmod from huge and tiny values: " << endl;
        auto r = t_extr.do_test(dummy_f);
        cout << "  Dummy function   :" << endl << test_fmod_base::report(r, 4) << endl;

        r = t_extr.do_test(fmod_libm);
        cout << "  Internal function:" << endl << test_fmod_base::report(r, 4) << endl;

        r = t_extr.do_test(my_fmod);
        cout << "  New function     :" << endl << test_fmod_base::report(r, 4) << endl;
    }

    return 0;
}