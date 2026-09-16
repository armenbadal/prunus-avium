SUB g(a[] AS TEXT, b[] AS TEXT)
    FOR i = 0 TO LEN(a)-1
        LET a[i] = b[i] & "_copy"
    END FOR
END SUB

SUB Main
    DIM x[2] AS TEXT
    LET x[0] = "One"
    LET x[1] = "Two"

    DIM y[2] AS TEXT
    CALL g x y

    FOR i = 0 TO LEN(y)-1
        CALL Print y[i]
    END FOR
END SUB
