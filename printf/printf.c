#include <stdio.h>
#include <stdarg.h>

int print_int(int number) {
  int result;
  int base;

  base = 1;

  if (number < 0) {
    result = putchar('-');
    base *= -1;
  }
  
  while(number / base > 10)
    base *= 10;

  do {
    result = putchar('0' + number / base);
    number = number % base;
    base /= 10;
  } while(base);

  return result;
}

int best_printf(const char *format, ...) {
  va_list args;
  int result;
  int int_arg;
  double double_arg;
  const char *string_arg;

  va_start(args, format);

  while (*format != '\0' && result != EOF) {
    if (*format == '%') {
      ++format;
      switch (*format) {
	case 'd':
	  int_arg = va_arg(args, int);
	  print_int(int_arg);
	  break;

	case 'f':
	  double_arg = va_arg(args, double);
	  printf("%f", double_arg);
	  break;

	case 's':
	  string_arg = va_arg(args, const char *);
	  fputs(string_arg, stdout);
	  break;

	case 'c':
	  int_arg = va_arg(args, int);
	  result = putchar((char) int_arg);
	  break;

	case '\0':
	  continue;
	  break;

	default:
	  result = putchar(*format);
	  break;
      }
    } else {
      result = putchar(*format);
    }

    ++format;
  }

  va_end(args);
  
  return result;
}
