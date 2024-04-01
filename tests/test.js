// single line comment

/*
multi
line
comment
*/

function strings() {
    "hello";
    'hello';
    `hello`;
}

function numbers() {
    // integers
    1;
    0x1;
    0b1;
    0o1;

    // floating-point
    1.2;
    .2;
    1.;
    1.2e3;
    .2e3;
    1.e3;
    1e3;
    1e+3;
    1e-3;

    // separators
    1_2;
}

function controlFlow(b) {
    if (b) {
        return;
    }
    else {
        for (let i = 0; i <= 9; i++) {
            continue;
        }
        while (b) {
            break;
        }
    }
}
