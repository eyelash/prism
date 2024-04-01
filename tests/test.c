// single line comment

/*
multi
line
comment
*/

static void strings() {
    "hello";
}

static void numbers() {
    // integers
    1u;
    0x1u;
    0b1u;

    // decimal floating-point
    1.2f;
    .2f;
    1.f;
    1.2e3f;
    .2e3f;
    1.e3f;
    1e3f;
    1e+3f;
    1e-3f;

    // hexadecimal floating-point
    0xA.Bp1f;
    0x.Bp1f;
    0xA.p1f;
    0xAp1f;
    0xAp+1f;
    0xAp-1f;

    // separators
    1'2;
}

static void control_flow(int b) {
    if (b) {
        return;
    }
    else {
        for (int i = 0; i <= 9; i++) {
            continue;
        }
        while (b) {
            break;
        }
    }
}

int main() {
    return 0;
}
