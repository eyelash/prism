// single line comment

/*
multi
line
comment
*/

fn strings() {
    "hello";
}

fn numbers() {
    // integers
    1;
    0x1;
    0b1;
    0o1;

    // floating-point
    1.2;
    1.;
    1.2e3;
    1e3;
    1e+3;
    1e-3;

    // separators
    1_2;
}

fn control_flow(b: bool) {
    if b {
        return;
    } else {
        for i in 0..=9 {
            continue;
        }
        while b {
            break;
        }
    }
}

fn main() {}
