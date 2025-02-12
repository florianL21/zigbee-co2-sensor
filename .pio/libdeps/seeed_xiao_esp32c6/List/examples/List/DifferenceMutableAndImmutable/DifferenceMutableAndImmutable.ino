#include <List.hpp>

// Create a mutable list
List<int> mutableList(true);
// Create an immutable list
List<int> immutableList;

void setup() {
  Serial.begin(9600);

  // create the first element
  int setup1 = 2;

  // add the element to both lists
  mutableList.add(setup1);
  immutableList.add(setup1);

  Serial.print("The element that is added to both lists is: ");
  Serial.println(setup1);

  // Get the first element of both lists
  Serial.print("The first element of the first list is: ");
  Serial.println(mutableList.get(0));
  Serial.print("The first element of the second list is: ");
  Serial.println(immutableList.get(0));

  Serial.println("All three values are logically the same!");
  Serial.println();

  // change the value of the setup1 variable
  setup1 = 9;

  Serial.print("The new value of the variable is: ");
  Serial.println(setup1);

  // Get the first element of both lists
  Serial.print("The first element of the first list is: ");
  Serial.println(mutableList.get(0));
  Serial.print("The first element of the second list is: ");
  Serial.println(immutableList.get(0));

  Serial.println("You see, that the first element of the first list has the the new value and the first element of the second list has not changed.");
  Serial.println("Elements of mutable lists can be manipulate via the same variable; immutable lists cannot!");
}

void loop() {}
