import std.io;
import std.math;

struct Person {
    str firstname;
    str lastname;
    int age;
    float weight;
    float height;
}

void main() {
    std.io.println("--- Standard Library Test ---");

    int a = 10;
    int b = 20;
    int m = std.math.max(a, b);

    std.io.println("Max of 10 and 20 is:");
    std.io.print_int(m);

    int p = std.math.pow(2, 10);
    std.io.println("2 power 10 is:");
    std.io.print_int(p);

    std.io.println("");

    Person person = new(Person);
    person.firstname = "Abrorbek";
    person.lastname = "Patidinov";
    person.age = 19;

    std.io.println("Person's firstname: %{str}", person.firstname);
    std.io.println("Person's lastname: %{str}", person.lastname);
    std.io.println("Person's age: %{int}", person.age);


    abyss_eye();
}
