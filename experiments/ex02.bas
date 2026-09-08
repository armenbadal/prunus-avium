SUB g(a[] AS TEXT, b[] AS TEXT)
    FOR i = 0 TO LEN(a)-1
        LET a[i] = b[i] & "_copy"
    END FOR
END SUB
