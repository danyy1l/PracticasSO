#include "pow.h"

#define PRIME POW_LIMIT
#define BIG_X 435679812
#define BIG_Y 100001819

u64 pow_hash(u64 x) {
  u64 result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}
/* Como se explica en la memoria, usamos datos unsigned porque el enunciado
 * especifica que la busqueda sea en el rango [0, POW_LIMIT -1], el operador %
 * de C no es el matematico y puede dar resultados negativos, por lo que
 * forzamos que el conjuntos de entrada y salida sean enteros mayores e iguales
 * a 0 */
