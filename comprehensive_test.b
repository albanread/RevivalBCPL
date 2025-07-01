MANIFEST $(
  MAX_VAL = 100
$)

LET helper(x) BE $(
  x := x + 1
$)

LET START() BE $(
  LET a = 5
  LET b = -10
  LET sum = a + b
  LET product = a * 3
  LET comparison = a > b
  a := sum + product
$)
