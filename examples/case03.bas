
SUB Main
  DIM a AS TEXT
  LET a = "345"

  DIM b AS BOOL
  LET b = NOT a

  DIM c AS REAL
  LET c = -a
  LET c = +a

  DIM d AS REAL
  LET d = a + c
  LET d = c - a
END SUB
