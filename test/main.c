#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "../src/expressions.h"
#include "../src/allocator.h"

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_BOLD    "\033[1m"

// Global test counters
static int total_tests = 0;
static int passed_tests = 0;

// Helper function to test expression evaluation and simplification
void test_expression(const char* test_name, expr_t expr, const char* expected_serialization, double expected_value_if_constant) {
    printf("=== Testing: %s ===\n", test_name);
    total_tests++;
    
    // Serialize original expression
    usize size = serialized_expr_size(&expr);
    char *buffer GPA_DEALLOC = (char*)gpa_allocator.alloc(sizeof(char) * (size + 1));
    buffer[size] = '\0'; // Null terminate
    
    serialize_expr(buffer, &expr);
    printf("Original: %s\n", buffer);
    
    // Simplify
    simplify(&expr);
    
    // Serialize simplified
    memset(buffer, 0, size + 1);
    serialize_expr(buffer, &expr);
    printf("Simplified: %s\n", buffer);
    
    bool test_passed = true;
    
    // Check if it matches expected serialization
    if (expected_serialization) {
        if (strcmp(buffer, expected_serialization) == 0) {
            printf("%s✓ Serialization matches expected: %s%s\n", 
                   COLOR_GREEN, expected_serialization, COLOR_RESET);
        } else {
            printf("%s✗ Expected: %s, Got: %s%s\n", 
                   COLOR_RED, expected_serialization, buffer, COLOR_RESET);
            test_passed = false;
        }
    }
    
    // Check if it's a constant with expected value
    if (expr.variant == EXPR_CONSTANT && expected_value_if_constant != INFINITY) {
        double diff = fabs(expr.constant - expected_value_if_constant);
        if (diff < 1e-10) {
            printf("%s✓ Value matches expected: %.6f%s\n", 
                   COLOR_GREEN, expected_value_if_constant, COLOR_RESET);
        } else {
            printf("%s✗ Expected value: %.6f, Got: %.6f (diff: %e)%s\n", 
                   COLOR_RED, expected_value_if_constant, expr.constant, diff, COLOR_RESET);
            test_passed = false;
        }
    }
    
    if (test_passed) {
        passed_tests++;
    }
    
    printf("\n");
}

int main() {
    printf("%s=== COMPREHENSIVE EXPRESSION LIBRARY TEST SUITE ===%s\n\n", 
           COLOR_BOLD COLOR_BLUE, COLOR_RESET);
    
    // Test constants
    test_expression("Constant 5", Const(5), "5", 5.0);
    test_expression("Constant π", Const(M_PI), NULL, M_PI);
    
    // Test variables (no simplification expected)
    expr_t var_x = Var('x');
    test_expression("Variable x", var_x, "x", INFINITY);
    
    // Test basic arithmetic
    test_expression("Addition: 3 + 2", Sum(&Const(3), &Const(2)), "5", 5.0);
                   
    test_expression("Subtraction: 7 - 3", Difference(&Const(7), &Const(3)), "4", 4.0);
                   
    test_expression("Multiplication: 4 * 6", Product(&Const(4), &Const(6)), "24", 24.0);
                   
    test_expression("Division: 15 / 3", Quotient(&Const(15), &Const(3)), "5", 5.0);
    
    // Test powers and exponentials
    test_expression("Power: 2^3", Power(&Const(2), &Const(3)), "8", 8.0);
                   
    test_expression("Power: 9^0.5", Power(&Const(9), &Const(0.5)), "3", 3.0);
                   
    test_expression("Exponential: e^2", Exponential(&Const(M_E), &Const(2)), NULL, exp(2.0));
    
    // Test logarithms
    test_expression("Natural log: ln(e^2)", Logarithm(&Const(M_E), &Power(&Const(M_E), &Const(2))), "2", 2.0);
                   
    test_expression("Log base 10: log₁₀(100)", Logarithm(&Const(10), &Const(100)), "2", 2.0);
                   
    test_expression("Log base 2: log₂(8)", Logarithm(&Const(2), &Const(8)), "3", 3.0);
    
    // Test trigonometric functions
    test_expression("sin(0)", Sin(&Const(0)), "0", 0.0);
                   
    test_expression("cos(0)", Cos(&Const(0)), "1", 1.0);
                   
    test_expression("sin(π/2)", Sin(&Const(M_PI/2)), "1", 1.0);
                   
    test_expression("cos(π)", Cos(&Const(M_PI)), "-1", -1.0);
                   
    test_expression("tan(π/4)", Tan(&Const(M_PI/4)), "1", 1.0);
    
    // Test negation
    test_expression("Negation: -5", Negation(&Const(5)), "-5", -5.0);
                   
    test_expression("Double negation: -(-3)", Negation(&Negation(&Const(3))), "3", 3.0);
    
    // Test inverse
    test_expression("Inverse: 1/4", Inverse(&Const(4)), "0.25", 0.25);
                   
    test_expression("Inverse of inverse: (1/(1/2))", Inverse(&Inverse(&Const(2))), "2", 2.0);
    
    // Test complex expressions
    test_expression("Complex: (2 + 3) * 4", Product(&Sum(&Const(2), &Const(3)), &Const(4)), "20", 20.0);
                   
    test_expression("Complex: 2^3 + 3^2", Sum(&Power(&Const(2), &Const(3)), &Power(&Const(3), &Const(2))), "17", 17.0);
                   
    test_expression("Complex: sin²(π/6) + cos²(π/6)", Sum(&Power(&Sin(&Const(M_PI/6)), &Const(2)), &Power(&Cos(&Const(M_PI/6)), &Const(2))), "1", 1.0);
    
    // Test expressions with variables (should not simplify to constants)
    expr_t x = Var('x');
    expr_t y = Var('y');
    
    printf("%s=== Testing expressions with variables (no simplification expected) ===%s\n", 
           COLOR_YELLOW, COLOR_RESET);
    
    expr_t var_expr1 = Sum(&x, &Const(0));
    test_expression("x + 0 (should stay as is)", var_expr1, NULL, INFINITY);
    
    expr_t var_expr2 = Product(&x, &Const(1));
    test_expression("x * 1 (should stay as is)", var_expr2, NULL, INFINITY);
    
    expr_t var_expr3 = Sum(&x, &y);
    test_expression("x + y", var_expr3, NULL, INFINITY);
    
    // Test edge cases
    printf("%s=== Testing edge cases ===%s\n", COLOR_YELLOW, COLOR_RESET);
    
    test_expression("Division by 1: 7/1", Quotient(&Const(7), &Const(1)), "7", 7.0);
                   
    test_expression("Multiplication by 0: 5*0", Product(&Const(5), &Const(0)), "0", 0.0);
                   
    test_expression("Power to 0: 5^0", Power(&Const(5), &Const(0)), "1", 1.0);
                   
    test_expression("Power to 1: 7^1", Power(&Const(7), &Const(1)), "7", 7.0);

    // Test nested expressions
    printf("%s=== Testing deeply nested expressions ===%s\n", COLOR_YELLOW, COLOR_RESET);

    expr_t nested = Sum(&Product(&Sin(&Const(M_PI/2)), &Cos(&Const(0))), &Power(&Const(2), &Logarithm(&Const(2), &Const(8))));

    test_expression("sin(π/2) * cos(0) + 2^(log₂(8))", nested, "9", 9.0);

    // Test your original complex expression
    printf("%s=== Testing original complex expression ===%s\n", COLOR_YELLOW, COLOR_RESET);
    
    expr_t original = Quotient(
        &Sum(&Var('p'), &Product(&Var('q'), &Sum(&Sin(&Const(6)), 
             &Sum(&Sum(&Const(5), &Product(&Const(3), &Inverse(&Const(2)))), &Const(5))))),
        &Inverse(&Inverse(&Inverse(&Const(8))))
    );
    test_expression("Original complex expression", original, NULL, INFINITY);
    
    // Print final summary
    printf("%s=== TEST SUITE COMPLETE ===%s\n", COLOR_BOLD COLOR_BLUE, COLOR_RESET);
    printf("Total tests run: %d\n", total_tests);
    printf("Tests passed: %s%d%s\n", COLOR_GREEN, passed_tests, COLOR_RESET);
    printf("Tests failed: %s%d%s\n", 
           (total_tests - passed_tests) > 0 ? COLOR_RED : COLOR_GREEN, 
           total_tests - passed_tests, COLOR_RESET);
    
    if (passed_tests == total_tests) {
        printf("\n%s🎉 ALL TESTS PASSED! 🎉%s\n", COLOR_BOLD COLOR_GREEN, COLOR_RESET);
    } else {
        printf("\n%s❌ SOME TESTS FAILED ❌%s\n", COLOR_BOLD COLOR_RED, COLOR_RESET);
        printf("Please review the failed tests above.\n");
    }
    
    printf("\nNote: Some tests may show small floating-point differences due to precision.\n");
    printf("Values marked with ∞ indicate expressions with variables that cannot be reduced to constants.\n");
    
    return (passed_tests == total_tests) ? 0 : 1;
}
