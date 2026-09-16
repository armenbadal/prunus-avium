SUB Main
    CALL Print STR(f(3))
END SUB

SUB f(a AS REAL) AS REAL
    DIM arr[5] AS TEXT
    IF a > 12 THEN
        RETURN a * a
    ELSE
        RETURN a^7
    END IF
END SUB
