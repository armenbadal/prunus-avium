SUB Main
    DIM a AS REAL
    DIM b AS TEXT
    DIM c AS BOOL

    DIM e[5] AS TEXT
    DIM f[2] AS REAL
    DIM g[8] AS BOOL

    LET g[0] = TRUE
    LET g[1] = FALSE
    LET g[2] = g[1]
    
END SUB

SUB sum(a[] AS REAL) AS REAL
    DIM s AS REAL
    FOR i = 0 TO LEN(a) - 1
        LET s = s + a[i]
    END FOR
    RETURN s
END SUB
