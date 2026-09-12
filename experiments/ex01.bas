SUB f(n AS REAL)
    DIM a[n*2] AS TEXT
    FOR i = 0 TO LEN(a) - 1
        LET a[i] = STR(i)
    END FOR
END SUB
