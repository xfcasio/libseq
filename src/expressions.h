#ifndef _LDS_EXPRESSIONS_H
#define _LDS_EXPRESSIONS_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "primitives.h"

struct __expr_t;

typedef struct { struct __expr_t* x; } unary_expr_t;
typedef struct { struct __expr_t* x, *y; } binary_expr_t;

typedef enum : u16 {
  EXPR_CONSTANT,
  EXPR_VARIABLE,

  EXPR_PRODUCT,
  EXPR_QUOTIENT,

  EXPR_SUM,
  EXPR_DIFFERENCE,

  EXPR_EXPONENTIAL,
  EXPR_LOGARITHM,
  EXPR_POWER,

  EXPR_SIN,
  EXPR_COS,
  EXPR_TAN,

  EXPR_NEGATION,
  EXPR_INVERSE
} expr_tag_t;

struct [[gnu::packed]] __expr_t {
  union {
    f64 constant;
    char variable;
    unary_expr_t arg;
    binary_expr_t args;
  };

  expr_tag_t variant;
};

typedef struct __expr_t expr_t;

#define Const(c) (expr_t) { .constant = (f64)c, .variant = EXPR_CONSTANT }
#define Var(c) (expr_t) { .variable = (char)c, .variant = EXPR_VARIABLE }

#define Product(a, b) (expr_t) { .args = (binary_expr_t){ .x = (a), .y = (b) }, .variant = EXPR_PRODUCT }
#define Quotient(a, b) (expr_t) { .args = (binary_expr_t){ .x = (a), .y = (b) }, .variant = EXPR_QUOTIENT }

#define Sum(a, b) (expr_t) { .args = (binary_expr_t){ .x = (a), .y = (b) }, .variant = EXPR_SUM }
#define Difference(a, b) (expr_t) { .args = (binary_expr_t){ .x = (a), .y = (b) }, .variant = EXPR_DIFFERENCE }

#define Exponential(a, b) (expr_t) { .args = (binary_expr_t){ .x = (a), .y = (b) }, .variant = EXPR_EXPONENTIAL }
#define Logarithm(base, e) (expr_t) { .args = (binary_expr_t){ .x = (base), .y = (e) }, .variant = EXPR_LOGARITHM }
#define Power(a, b) (expr_t) { .args = (binary_expr_t){ .x = (a), .y = (b) }, .variant = EXPR_POWER }

#define Sin(e) (expr_t) { .arg = (unary_expr_t){e}, .variant = EXPR_SIN }
#define Cos(e) (expr_t) { .arg = (unary_expr_t){e}, .variant = EXPR_COS }
#define Tan(e) (expr_t) { .arg = (unary_expr_t){e}, .variant = EXPR_TAN }

#define Negation(e) (expr_t) { .arg = (unary_expr_t){e}, .variant = EXPR_NEGATION }
#define Inverse(e) (expr_t) { .arg = (unary_expr_t){e}, .variant = EXPR_INVERSE }


static usize count_serialized_expr_size(expr_t *e, usize depth) {
  expr_tag_t variant = e->variant;
  bool parens_condition = (depth > 0) &&
    (variant != EXPR_CONSTANT) &&
    (variant != EXPR_VARIABLE) &&
    (variant != EXPR_PRODUCT) &&
    (variant != EXPR_LOGARITHM) &&
    (variant != EXPR_SIN) &&
    (variant != EXPR_COS) &&
    (variant != EXPR_TAN);

  usize offset_written = 0;

  if (parens_condition) offset_written += 2;

  switch (variant) {
    case EXPR_CONSTANT: {
      offset_written += snprintf(NULL, 0, "%.3g", e->constant);
      break;
    }
    case EXPR_VARIABLE: {
      offset_written++;
      break;
    }
    case EXPR_PRODUCT: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;

      if (((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_VARIABLE)) || (x_variant == EXPR_SUM)) {
        // 
        // 
        //  WHY A LOT OF BRANCHING
        // 
        // 
        offset_written += count_serialized_expr_size(e->args.x, depth + 1);
        offset_written += count_serialized_expr_size(e->args.y, depth + 1);
      } else if (((x_variant == EXPR_VARIABLE) && (y_variant == EXPR_CONSTANT)) || (y_variant == EXPR_SUM)) {
        offset_written += count_serialized_expr_size(e->args.y, depth + 1);
        offset_written += count_serialized_expr_size(e->args.x, depth + 1);
      } else {
        offset_written += count_serialized_expr_size(e->args.x, depth + 1);
        offset_written++;
        offset_written += count_serialized_expr_size(e->args.y, depth + 1);
      }
      break;
    }

    case EXPR_QUOTIENT:
    case EXPR_SUM:
    case EXPR_DIFFERENCE:
    case EXPR_EXPONENTIAL:
    case EXPR_POWER: {
      offset_written += count_serialized_expr_size(e->args.x, depth + 1);
      offset_written++;
      offset_written += count_serialized_expr_size(e->args.y, depth + 1);
      break;
    }

    case EXPR_LOGARITHM: {
      offset_written += 3 /* log */
        + count_serialized_expr_size(e->args.x, depth + 1)
        + count_serialized_expr_size(e->args.y, depth + 1) + 3 ;// 1 (comma) + 2(parens)

      break;
    }

    case EXPR_SIN:
    case EXPR_COS:
    case EXPR_TAN: {
      offset_written += 5 /* 3 (sin/cos/tan) + 2 (parens) */
        + count_serialized_expr_size(e->arg.x, depth + 1);

      break;
    }

    case EXPR_NEGATION: {
      offset_written++;
      offset_written += count_serialized_expr_size(e->arg.x, depth + 1);
      break;
    }

    case EXPR_INVERSE: {
      offset_written += count_serialized_expr_size(e->arg.x, depth + 1);
      offset_written += strlen("⁻¹");
      break;
    }

    default:
      puts("count_serialized_expr_size: corrupted/unhandled expression variant");
      abort();
  }

  return offset_written;
}

static usize counted_serialize_expr(char *serialization_buffer, expr_t *e, usize depth) {
  expr_tag_t variant = e->variant;
  bool parens_condition = (depth > 0) &&
    (variant != EXPR_CONSTANT) &&
    (variant != EXPR_VARIABLE) &&
    (variant != EXPR_PRODUCT) &&
    (variant != EXPR_LOGARITHM) &&
    (variant != EXPR_SIN) &&
    (variant != EXPR_COS) &&
    (variant != EXPR_TAN);

  usize offset_written = 0;

  if (parens_condition) serialization_buffer[offset_written++] = '(';

  switch (variant) {
    case EXPR_CONSTANT: {
      i32 num_of_digits;

      num_of_digits = snprintf(NULL, 0, "%.3g", e->constant);
      sprintf(serialization_buffer + offset_written, "%.3g", e->constant);

      offset_written += num_of_digits;
      break;
    }
    case EXPR_VARIABLE: {
      sprintf (serialization_buffer + offset_written, "%c", e->variable);
      offset_written++;
      break;
    }
    case EXPR_PRODUCT: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;

      if (((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_VARIABLE)) || (x_variant == EXPR_SUM)) {
        offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
        offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);
      } else if (((x_variant == EXPR_VARIABLE) && (y_variant == EXPR_CONSTANT)) || (y_variant == EXPR_SUM)) {
        offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);
        offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
      } else {
        offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
        serialization_buffer[offset_written++] = '*';
        offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);
      }
      break;
    }

    case EXPR_QUOTIENT: {
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
      serialization_buffer[offset_written++] = '/';
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);
      break;
    }

    case EXPR_SUM: {
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
      serialization_buffer[offset_written++] = '+';
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);
      break;
    }

    case EXPR_DIFFERENCE: {
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
      serialization_buffer[offset_written++] = '-';
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);
      break;
    }

    case EXPR_EXPONENTIAL:
    case EXPR_POWER: {
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
      serialization_buffer[offset_written++] = '^';
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);
      break;
    }

    case EXPR_LOGARITHM: {
      memcpy(serialization_buffer + offset_written, "log(", strlen("log("));
      offset_written += strlen("log(");

      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.x, depth + 1);
      serialization_buffer[offset_written++] = ',';
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->args.y, depth + 1);

      serialization_buffer[offset_written++] = ')';
      break;
    }

    case EXPR_SIN: {
      memcpy(serialization_buffer + offset_written, "sin(", strlen("sin("));
      offset_written += strlen("sin(");

      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->arg.x, depth + 1);

      serialization_buffer[offset_written++] = ')';
      break;
    }


    case EXPR_COS: {
      memcpy(serialization_buffer + offset_written, "cos(", strlen("cos("));
      offset_written += strlen("cos(");

      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->arg.x, depth + 1);

      serialization_buffer[offset_written++] = ')';
      break;
    }

    case EXPR_TAN: {
      memcpy(serialization_buffer + offset_written, "tan(", strlen("tan("));
      offset_written += strlen("tan(");

      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->arg.x, depth + 1);

      serialization_buffer[offset_written++] = ')';
      break;
    }

    case EXPR_NEGATION: {
      serialization_buffer[offset_written++] = '-';
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->arg.x, depth + 1);
      break;
    }

    case EXPR_INVERSE: {
      offset_written += counted_serialize_expr(serialization_buffer + offset_written, e->arg.x, depth + 1);
      memcpy(serialization_buffer + offset_written, "⁻¹", strlen("⁻¹"));
      offset_written += strlen("⁻¹");
      break;
    }

    default:
      puts("counted_serialize_expr: corrupted/unhandled expression variant");
      abort();
  }

  if (parens_condition) serialization_buffer[offset_written++] = ')';

  return offset_written;
}

usize serialized_expr_size(expr_t *e) {
  return count_serialized_expr_size(e, 0);
}

usize serialize_expr(char *buffer, expr_t *e) {
  usize written = counted_serialize_expr(buffer, e, 0);
  return written;
}

bool is_binary_expression(expr_t *e) {
  switch (e->variant) {
    case EXPR_CONSTANT:
    case EXPR_VARIABLE:
    case EXPR_NEGATION:
    case EXPR_SIN:
    case EXPR_COS:
    case EXPR_TAN:
    case EXPR_INVERSE: { return false; }

    case EXPR_PRODUCT:
    case EXPR_QUOTIENT:
    case EXPR_SUM:
    case EXPR_DIFFERENCE:
    case EXPR_EXPONENTIAL:
    case EXPR_LOGARITHM:
    case EXPR_POWER: { return true; }

    default:
      puts("is_binary_expression: corrupted/unhandled expression variant");
      abort();
  }
}

bool is_simplifiable(expr_t *e) {
  switch (e->variant) {
    case EXPR_CONSTANT:
    case EXPR_VARIABLE: { return false; }

    case EXPR_PRODUCT:
    case EXPR_QUOTIENT:
    case EXPR_SUM:
    case EXPR_DIFFERENCE:
    case EXPR_POWER:
    case EXPR_LOGARITHM:
    case EXPR_EXPONENTIAL: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;

      if ((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_CONSTANT)) return true;
      else return is_simplifiable(e->args.x) || is_simplifiable(e->args.y);
    }

    case EXPR_NEGATION: {
      if (e->arg.x->variant == EXPR_CONSTANT) return false;
      else return is_simplifiable(e->arg.x);
    }

    case EXPR_SIN:
    case EXPR_COS:
    case EXPR_TAN:
    case EXPR_INVERSE: {
      if (e->arg.x->variant == EXPR_CONSTANT) return true;
      else return is_simplifiable(e->arg.x);
    }

    default:
      puts("is_simplifiable: corrupted/unhandled expression variant");
      abort();
  }
}

void simplify(expr_t *e) {
  switch (e->variant) {
    case EXPR_CONSTANT:
    case EXPR_VARIABLE: break;

    case EXPR_PRODUCT: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;
      expr_t *x = e->args.x;
      expr_t *y = e->args.y;

      if ((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_CONSTANT)) {
        *e = Const(x->constant * y->constant);
        break;
      }

      simplify(x);
      simplify(y);

      expr_t product = Product(x, y);
      
      if (is_simplifiable(&product)) simplify(&product);

      *e = product;
      break;
    }

    case EXPR_QUOTIENT: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;
      expr_t *x = e->args.x;
      expr_t *y = e->args.y;

      if ((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_CONSTANT)) {
        *e = Const(x->constant / y->constant);
        break;
      }

      simplify(x);
      simplify(y);

      expr_t quotient = Quotient(x, y);

      if (is_simplifiable(&quotient)) simplify(&quotient);

      *e = quotient;
      break;
    }

    case EXPR_SUM: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;
      expr_t *x = e->args.x;
      expr_t *y = e->args.y;

      if ((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_CONSTANT)) {
        *e = Const(x->constant + y->constant);
        break;
      }

      simplify(x);
      simplify(y);

      expr_t sum = Sum(x, y);
      if (is_simplifiable(&sum)) simplify(&sum);

      *e = sum;
      break;
    }

    case EXPR_DIFFERENCE: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;
      expr_t *x = e->args.x;
      expr_t *y = e->args.y;

      if ((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_CONSTANT)) {
        *e = Const(x->constant - y->constant);
        break;
      }

      simplify(x);
      simplify(y);

      expr_t difference = Difference(x, y);
      if (is_simplifiable(&difference)) simplify(&difference);

      *e = difference;
      break;
    }

    case EXPR_EXPONENTIAL: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;
      expr_t *x = e->args.x;
      expr_t *y = e->args.y;

      if ((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_CONSTANT)) {
        *e = Const(pow(x->constant, y->constant));
        break;
      }

      simplify(x);
      simplify(y);

      expr_t exponential = Exponential(x, y);
      if (is_simplifiable(&exponential)) simplify(&exponential);

      *e = exponential;
      break;
    }

    case EXPR_LOGARITHM: {
      expr_tag_t base_variant = e->args.x->variant;
      expr_tag_t expr_variant = e->args.y->variant;
      expr_t *base = e->args.x;
      expr_t *expr = e->args.y;

      if ((base_variant == EXPR_CONSTANT) && (expr_variant == EXPR_CONSTANT)) {
        *e = Const(log(expr->constant) / log(base->constant));
        break;
      }

      simplify(base);
      simplify(expr);

      expr_t logarithm = Logarithm(base, expr);
      if (is_simplifiable(&logarithm)) simplify(&logarithm);

      *e = logarithm;
      break;
    }

    case EXPR_POWER: {
      expr_tag_t x_variant = e->args.x->variant;
      expr_tag_t y_variant = e->args.y->variant;
      expr_t *x = e->args.x;
      expr_t *y = e->args.y;

      if ((x_variant == EXPR_CONSTANT) && (y_variant == EXPR_CONSTANT)) {
        *e = Const(pow(x->constant, y->constant));
        break;
      }

      simplify(x);
      simplify(y);

      expr_t power = Power(x, y);
      if (is_simplifiable(&power)) simplify(&power);

      *e = power;
      break;
    }

    case EXPR_SIN: {
      expr_tag_t x_variant = e->arg.x->variant;
      expr_t *x = e->arg.x;

      if (x_variant == EXPR_CONSTANT) {
        *e = Const(sin(x->constant));
        break;
      }

      simplify(x);

      expr_t sin = Sin(x);
      if (is_simplifiable(&sin)) simplify(&sin);

      *e = sin;
      break;
    }

    case EXPR_COS: {
      expr_tag_t x_variant = e->arg.x->variant;
      expr_t *x = e->arg.x;

      if (x_variant == EXPR_CONSTANT) {
        *e = Const(cos(x->constant));
        break;
      }

      simplify(x);

      expr_t cos = Cos(x);
      if (is_simplifiable(&cos)) simplify(&cos);

      *e = cos;
      break;
    }

    case EXPR_TAN: {
      expr_tag_t x_variant = e->arg.x->variant;
      expr_t *x = e->arg.x;

      if (x_variant == EXPR_CONSTANT) {
        *e = Const(tan(x->constant));
        break;
      }

      simplify(x);

      expr_t tan = Tan(x);
      if (is_simplifiable(&tan)) simplify(&tan);

      *e = tan;
      break;
    }

    case EXPR_NEGATION: {
      expr_t *x = e->arg.x;
      simplify(x);

      expr_t negation = Negation(x);
      if (is_simplifiable(&negation)) simplify(&negation);

      *e = negation;
      break;
    }

    case EXPR_INVERSE: {
      expr_tag_t x_variant = e->arg.x->variant;
      expr_t *x = e->arg.x;

      if (x_variant == EXPR_CONSTANT) {
        *e = Const((f64)1.0 / x->constant);
        break;
      }

      simplify(x);

      expr_t inverse = Inverse(x);
      if (is_simplifiable(&inverse)) simplify(&inverse);

      *e = inverse;
      break;
    }
  }
}

#endif
