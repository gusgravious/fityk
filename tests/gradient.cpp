
#include <stdio.h>
#include <boost/scoped_ptr.hpp>
#include "fityk/logic.h"
#include "fityk/data.h"
#include "fityk/fit.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace std;
using namespace fityk;


// Box and Betts exponential quadratic sum, equivalent to least-squares of
// f(x) = exp(-0.1*a0*x) - exp(-0.1*a1*x) - a2 * (exp(-0.1*x) - exp(-x))
// with points (1,0), (2,0), ... (10,0)
// 
// domains of a0, a1, a2 are, respectively, (0.9, 1.2), (9, 11.2), (0.9, 1.2)
// minimum: (1,10,1) -> 0

// modified boxbetts_f() from nlopt-2.3/test/testfuncs.c
static double boxbetts_f(const double *a, double *grad)
{
    double f = 0;
    if (grad)
        grad[0] = grad[1] = grad[2] = 0;
    for (int x = 1; x <= 10; ++x) {
        double e0 = exp(-0.1*x*a[0]);
        double e1 = exp(-0.1*x*a[1]);
        double e2 = exp(-0.1*x) - exp(-x);
        double g = e0 - e1 - e2 * a[2];
        f += g*g;
        if (grad) {
            grad[0] += (2 * g) * (-0.1*x*e0);
            grad[1] += (2 * g) * (0.1*x*e1);
            grad[2] += -(2 * g) * e2;
        }
    }
    return f;
}

static double boxbetts_in_fityk(const double *a, double *grad)
{
    boost::scoped_ptr<Ftk> ftk(new Ftk);
    ftk->settings_mgr()->set_as_number("verbosity", -1);
    for (int i = 1; i <= 10; ++i)
        ftk->get_data(0)->add_one_point(i, 0, 1);
    ftk->ui()->raw_execute_line("define BoxBetts(a0,a1,a2) = "
            "exp(-0.1*a0*x) - exp(-0.1*a1*x) - a2 * (exp(-0.1*x) - exp(-x))");
    ftk->ui()->raw_execute_line("F = BoxBetts(~0.9, ~11.8, ~1.08)");

    ftk->get_fit()->get_dof(ftk->get_dms()); // to invoke update_par_usage()
    vector<realt> avec(a, a+3);
    return ftk->get_fit()->compute_wssr_gradient(avec, ftk->get_dms(), grad);
}


int test_gradient()
{
    const double a[3] = { 0.9, 11.8, 1.08 };
    double grad[3], grad_again[3];
    double ssr = boxbetts_f(a, grad);
    printf("ssr=%.10g  grad: %.10g %.10g %.10g\n",
            ssr, grad[0], grad[1], grad[2]);

    double ssr_again = boxbetts_in_fityk(a, grad_again);
    printf("ssr=%.10g  grad: %.10g %.10g %.10g\n",
            ssr_again, grad_again[0], grad_again[1], grad_again[2]);
    return 0;
}
//int main() { return test_gradient(); }


TEST_CASE("set-throws", "test Fityk::set_throws()") {
    boost::scoped_ptr<Fityk> fik(new Fityk);
    REQUIRE(fik->get_throws() == true);
    REQUIRE_THROWS_AS(fik->execute("hi"), SyntaxError);
    REQUIRE_THROWS_AS(fik->execute("fit"), ExecuteError);
    fik->set_throws(false);
    REQUIRE_NOTHROW(fik->execute("hi"));
    fik->execute("hi");
    REQUIRE(fik->last_error() != "");
    REQUIRE(fik->last_error().substr(0,12) == "SyntaxError:");
    fik->clear_last_error();
    REQUIRE(fik->last_error() == "");
}

TEST_CASE("gradient", "test Fit::compute_wssr_gradient()") {
    const double a[3] = { 0.9, 11.8, 1.08 };
    double grad[3], grad_again[3];
    double ssr = boxbetts_f(a, grad);
    double ssr_again = boxbetts_in_fityk(a, grad_again);
    REQUIRE(ssr == Approx(ssr_again));
    REQUIRE(grad[0] == Approx(grad_again[0]));
    REQUIRE(grad[1] == Approx(grad_again[1]));
    REQUIRE(grad[2] == Approx(grad_again[2]));
}

