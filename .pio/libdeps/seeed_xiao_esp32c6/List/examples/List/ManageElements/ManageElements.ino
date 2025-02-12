#include <List.hpp>

// Create an immutable list
List<int> list;

// TODO: rvalue
void setup() {
  Serial.begin(9600);

  // Add the first element to the list
  int setup1 = 25;
  list.add(setup1);// add() will always place the element at the end of the list. Its synonym is addLast(). addFirst() will place the element directly at the front of the list.
  Serial.println("Inserted the first element");

  Serial.println();

  // Get the first element
  Serial.print("The value of the first element with the getValue() method is: ");
  Serial.println(list.get(0));// The most comfortable way to get the first elements value is to call the getValue() method. You cannot get the address of the element with the getValue() method!

  // Get the first element (alternative)
  Serial.print("The value of the first element with the [] operator is: ");
  int firstElement = list[0];// The '[]' Operator is a synonym for the getValue() method.
  Serial.println(firstElement);

  // TODO: add getMutableValue for mutable lists

  Serial.println("As you can see, there are three possible ways to get the value. The last way is not for beginners because you have to handle pointers.");
  Serial.println();

  // Get the size of the list
  Serial.print("The list has: ");
  Serial.print(list.getSize());
  Serial.println(" element(s)");

  Serial.println("Adding 10 more elements");
  // Add 10 more elements
  int y = 0;
  for (int i = 0; i < 10; ++i) {
    list.add(y);
    y++;
  }

  // Remove element from list
  list.remove(3);// With this, you will remove the third element of the list.
  Serial.print("After the deletion of the third element, the list has: ");
  Serial.print(list.getSize());
  Serial.println(" element(s)");

  // Remove the first and the last element
  list.removeFirst();
  list.removeLast();
  Serial.print("After the deletion of the first and the last element, the list has: ");
  Serial.print(list.getSize());
  Serial.println(" element(s)");

  // Clear list
  list.clear();
  Serial.print("After the clear, the list has: ");
  Serial.print(list.getSize());
  Serial.println(" element(s)");
}

void loop() {}
