// single line comment

/*
multi
line
comment
*/

public class Test {
    public static void controlFlow(boolean b) {
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

    public static void main(String[] args) {}
}
