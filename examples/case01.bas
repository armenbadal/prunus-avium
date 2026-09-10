
SUB max(x AS REAL, y AS REAL) AS REAL
  DIM t AS REAL

  IF x + y > 0 THEN
    LET t = 100
  END IF

  DIM sum AS REAL
  LET sum = x + y

  IF x > y THEN
    RETURN x
  ELSE
    RETURN y
  END IF
END SUB

SUB Main
  DIM m1 AS REAL
  LET m1 = max(6, 1981)
  DIM m2 AS REAL
  LET m2 = max(6, 1981)
END SUB
