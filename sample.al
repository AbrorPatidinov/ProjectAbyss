void main() {
    print("");

    print("Hello ! from abysslang !");

    int a = 50;
    str fullname = "Abrorbek Patidinov";
    float weight = 3.14;

    print("Integer: %{integer}", a);
    print("String: %{string}", fullname);
    print("Weight: %{float}", weight);

    print("");

    if (a > 90) {
        print("Integer %{integer} is higher than 90!", a);
    } else {
        print("Nice number though, which is %{integer}", a);
    }

    int i = 5;
    while (i > 5) {
        print("Numbers: %{integer}", i);
        i++; // i++; is not working as well, lets add it
    }

    print("");
    abyss_eye();
}
