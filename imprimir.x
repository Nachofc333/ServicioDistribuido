program IMPRIMIR {
    version IMPRIMIR_V1 {
        int IMPRIMIR_NF(string op<>, string fecha<>, string hora<>, string user<>) = 1;
        int IMPRIMIR_F(string op<>, string fecha<>, string hora<>, string user<>, string file<>) = 2;
    } = 1;
} = 99;