
SUB IsPrime(n AS REAL) AS BOOL
  IF n <= 2 THEN
    RETURN TRUE
  ELSEIF n MOD 2 = 0 THEN
    RETURN FALSE
  ELSE
    DIM k AS REAL
    LET k = 1 + SQR(n)
    WHILE (n MOD k <> 0) AND (k > 2)
      LET k = k - 1
    END WHILE
    RETURN k <> 2
  END IF
END SUB

SUB Main
  DIM e0 AS BOOL
  LET e0 = IsPrime(17)
  DIM s AS TEXT
  IF e0 THEN
    LET s = "Yes"
  END IF

  IF IsPrime(16) THEN
    LET s = "Yes"
  ELSE
    LET s = "No"
  END IF
END SUB
