public class Test {
    // single line comment

    /*
    multi
    line
    comment
    */

    public static void strings(String s) {
        s = "hello";
    }

    public static void numbers(Number n) {
        // integers
        n = 1;
        n = 0x1;
        n = 0b1;

        // decimal floating-point
        n = 1.2;
        n = .2;
        n = 1.;
        n = 1.2e3;
        n = .2e3;
        n = 1.e3;
        n = 1e3;
        n = 1e+3;
        n = 1e-3;

        // hexadecimal floating-point
        n = 0xA.Bp1;
        n = 0x.Bp1;
        n = 0xA.p1;
        n = 0xAp1;
        n = 0xAp+1;
        n = 0xAp-1;

        // separators
        n = 1_2;
    }

    public static void controlFlow(boolean b, int n) {
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
        switch (n) {
            case 0:
                break;
            default:
                break;
        }
    }

    public static void main(String[] args) {}
}
