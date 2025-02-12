/*!
 * @file AbstractList.hpp
 *
 * This file is part of the List library. It extends the arduino ecosystem with
 * easy-to-use list implementations. They are specially designed and optimized
 * for different purposes.
 *
 * Copyright (C) 2021-2024  Niklas Kaaf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef LIST_ABSTRACT_LIST_HPP
#define LIST_ABSTRACT_LIST_HPP

#include <stddef.h>

/*!
 * @brief   Abstract class from which all lists can be derived.
 *
 * @tparam T    Data Type of entries, that should be stored in the list.
 */
template<typename T>
class AbstractList {
  size_t size = 0;         /// Size of the list.
  bool mutableList = false;/// Is the list mutable or immutable.

 protected:
  /// Sometimes it is allowed, that index == this->getSize() to insert it behind
  /// the last entry
#define extendedIsIndexOutOfBounds(index) \
  ((index) != this->getSize() && this->isIndexOutOfBounds(index))

  /**
   * Class representing an abstract entry in the list.
   */
  class AbstractEntry {
    T immutableValue;         /// The raw value, assigned for immutable lists.
    T *mutableValue = nullptr;/// A pointer to the raw value, assigned for mutable lists.

   public:
    /*!
     * @brief   Get a pointer to the value (mutable or immutable).
     *
     * @param m   Indicates, if the list is mutable or immutable.
     *
     * @return  Pointer to the value of the entry.
     */
    T *getValue(const bool m) {
      if (m) {
        return mutableValue;
      }
      return &immutableValue;
    }

    /*!
     * @brief   Set the value.
     *
     * @param val   Reference to the value.
     * @param m   Indicates, if the list is mutable or immutable.
     */
    void setValue(T &val, const bool m) {
      if (m) {
        mutableValue = &val;
      } else {
        immutableValue = val;
      }
    }
  };

  /*!
   * @brief Constructor of an AbstractList Object.
   *
   * @param mutableList true if the list should be mutable; false otherwise.
   */
  explicit AbstractList<T>(const bool mutableList) : mutableList(mutableList) {}

  /*!
   * @brief Get a pointer to the element, stored at specific index.
   *
   * @param index The index of the element to retrieve.
   * @return The nullptr, if the index is out of bounds, otherwise the address
   * of the element.
   *
   * @note  This is independent from the mutability of the list. It will always return the
   * correct address (pointer) to the element.
   * @note Allowed indices are 0 to getSize() -1. If the index is out of bounds,
   * a nullptr will be returned.
   */
  virtual T *getPointer(int index) = 0;

  /*!
   * @brief Increase the size of the list by one. Should only be called after an
   *        insertion!
   */
  void increaseSize() { size++; }

  /*!
   * @brief Decrease the size of the list by one. Should only be called after an
   *        deletion!
   */
  void decreaseSize() { size--; }

  /*!
   * @brief Reset the size to zero.
   */
  void resetSize() { size = 0; }

  /*!
   * @brief Method to verify if the given index is out of the range of the list
   *        size.
   *
   * @param index   Index to check.
   * @return    true if the given index is in the range of the list; false
   *            otherwise
   */
  bool isIndexOutOfBounds(const int index) const { return index < 0 || index >= getSize(); }

 public:
  /*!
   * @copydoc AbstractList::addLast()
   * @note Alias of addLast().
   * @note  If this list is mutable, ensure, that all variables added to the lists do not go out-of-scope during all operations of the list.
   * @see addLast()
   */
  void add(T &value) { addLast(value); }

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  /*!
   * @copydoc AbstractList::add()
   * @note If the list is mutable and the values saved in the list are not primitives, nothing happen.
   */
  void add(T &&value) { addLast(value); }
#endif

  /*!
   * @brief Add the value to the list at the given index. The original entry at
   *        this index, and followings, will be placed directly after the new
   *        entry.
   * @note  Allowed indices are 0 to getSize(). If the index is out of bounds,
   * nothing will happen.
   * @note  If this list is mutable, ensure, that all variables added to the lists do not go out-of-scope during all operations of the list.
   *
   * @param index   Index of the entry, where the value should be added.
   * @param value   Value of the new entry.
   */
  virtual void addAtIndex(int index, T &value) = 0;

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  /*!
   * @copydoc AbstractList::addAtIndex()
   * @note If the list is mutable nothing happen.
   */
  virtual void addAtIndex(const int index, T &&value) {
    if (this->isMutable()) {
      return;// Mutable lists cannot save rvalues!
    }
    addAtIndex(index, value);
  }
#endif

  /*!
   * @brief Add all entries from the given list to this list at a specified
   * index. The original entry at this index, and followings, will be placed
   *        directly after the entries of the given list.
   * @note The elements from the other list, remain untouched.
   * @note  If the other list is mutable and the values saved in the lists are primitives, nothing happens.
   *
   * @param index   Index of this list, at which all entries should be added.
   * @param list    Other list from where to copy the entries.
   */
  void addAll(int index, AbstractList<T> &list) {
    for (int i = 0; i < list.getSize(); i++) {
      addAtIndex(index++, list.get(i));
    }
  }

  /*!
   * @brief Add all entries from the given list at the end of the list.
   * @note The elements from the other list, remain untouched.
   * @note  If the other list is mutable and the values saved in the lists are primitives, nothing happens.
   *
   * @param list    Other list to copy from.
   */
  void addAll(AbstractList<T> &list) { addAll(getSize(), list); }

  /*!
   * @brief Add all entries from an array to this list at a specified index.
   *        The original entry at this index, and followings, will be placed
   *        directly after the entries of the given list.
   * @note The elements from the other list, remain untouched.
   * @note  If this list is mutable, ensure, that the other lists do not go out-of-scope during all operations of this list. If the other list is mutable, all values added to this lists are immutable.
   *
   * @param index Index of this list, at which all entries should be added.
   * @param arr Array.
   * @param arrSize Size of the array.
   */
  void addAll(int index, T *arr, const size_t arrSize) {
    for (size_t i = 0; i < arrSize; ++i) {
      addAtIndex(index++, arr[i]);
    }
  }

  /*!
   * @brief Add all entries from an array.
   * @note The elements from the other list, remain untouched.
   *
   * @param arr Array.
   * @param arrSize  Size of array.
   */
  void addAll(T *arr, const size_t arrSize) {
    for (size_t i = 0; i < arrSize; ++i) {
      add(arr[i]);
    }
  }

  /*!
   * @brief Add a new entry at the beginning of the list.
   * @note  If this list is mutable, ensure, that all variables added to the lists do not go out-of-scope during all operations of the list.
   *
   * @param value   Value to add.
   */
  void addFirst(T &value) { addAtIndex(0, value); }

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  /*!
   * @copydoc AbstractList::addFirst()
   * @note If the list is mutable and the values saved in the list are not primitives, nothing happen.
   */
  void addFirst(T &&value) { addAtIndex(0, value); }
#endif

  /*!
   * @brief Add a new entry at the end of the list.
   * @note  If this list is mutable, ensure, that all variables added to the lists do not go out-of-scope during all operations of the list.
   *
   * @param value   Value to add.
   */
  void addLast(T &value) { addAtIndex(getSize(), value); }

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  /*!
   * @copydoc AbstractList::addLast()
   * @note If the list is mutable and the values saved in the list are not primitives, nothing happen.
   */
  void addLast(T &&value) { addAtIndex(getSize(), value); }
#endif

  /*!
   * @brief Get the raw value at a specified index.
   *
   * @note Allowed indices are 0 to getSize() - 1. If the index is out of
   * bounds, undefined behaviour will happen. Please ne sure, that the index is
   * valid!
   * @note This will method will always return an immutable object. If you want
   * to get the mutable object from your mutable list, please use
   * getMutableValue().
   *
   * @param index   Index of the element to get.
   * @return    Immutable object.
   */
  T get(const int index) { return *this->getPointer(index); }

  /*!
   * @brief Get the pointer to the mutable object at a specified index.
   *
   * @note Allowed indices are 0 to getSize() -1. If the index is out of bounds,
   * a nullptr will be returned.
   * @note This method will only return a valid object for a mutable list. A
   * immutable list will return always the nullptr.
   *
   * @param index Index of teh element to get.
   * @return Mutable object, or nullptr if the index is out of bounds or the
   * list is immutable.
   */
  T *getMutableValue(const int index) {
    if (!this->isMutable()) {
      return nullptr;
    }

    return this->getPointer(index);
  }

  /*!
   * @brief Remove all elements from the List.
   */
  virtual void clear() = 0;

  /*!
   * @brief Remove the entry at the given index.
   *
   * @note Allowed indices are 0 to getSize() - 1. If the index is out of
   * bounds, nothing will happen.
   *
   * @param index   Index of element to remove.
   */
  virtual void remove(int index) = 0;

  /*!
   * @brief Remove the first entry from the list.
   */
  virtual void removeFirst() { remove(0); }

  /*!
   * @brief Remove the las entry from the list.
   */
  virtual void removeLast() { remove(getSize() - 1); }

  /*!
   * @copydoc AbstractList::clear()
   * @note Alias of clear().
   * @see clear()
   */
  void removeAll() { clear(); }

  /*!
   * @brief Get the number how many elements are saved in the list.
   *
   * @return    Size of the list.
   */
  int getSize() const { return size; }

  /*!
   * @brief Check if the list is mutable.
   *
   * @return    true if the list is mutable; false otherwise.
   */
  bool isMutable() const { return mutableList; }

  /*!
   * @brief Check if the list is empty.
   *
   * @return    true if the list is empty; false otherwise
   */
  bool isEmpty() const { return getSize() == 0; }

  /*!
   * @brief Compare two lists whether their attributes and entries are equal.
   * @note  If you use this list for non-primitive data types, check if the
   *        data type implements the != operator!
   *
   * @param other    Second list to compare.
   * @return    true if the lists are equal; false otherwise.
   */
  bool equals(AbstractList<T> &other) {
    if (other.isMutable() != this->isMutable()) {
      return false;
    }

    if (other.getSize() != this->getSize()) {
      return false;
    }

    for (int i = 0; i < getSize(); i++) {
      if (this->isMutable()) {
        if (other.getMutableValue(i) != this->getMutableValue(i)) {
          return false;
        }
      } else {
        if (other.get(i) != this->get(i)) {
          return false;
        }
      }
    }
    return true;
  }

  /*!
   * @copydoc AbstractList::get()
   * @see   get()
   */
  T operator[](const int index) { return get(index); }

  /*!
   * @copydoc AbstractList::equals()
   * @see   equals()
   */
  bool operator==(AbstractList<T> &other) { return equals(other); }

  /*!
   * @brief Opposite of '=='
   * @see equals()
   *
   * @param other Other list to compare
   * @return    true if the lists are not equal; false otherwise.
   */
  bool operator!=(AbstractList<T> &other) { return !this->equals(other); }

  /*!
   * @copydoc AbstractList::add()
   * @note  If this list is mutable, ensure, that all variables added to the lists do not go out-of-scope during all operations of the list.
   * @see add()
   */
  void operator+(T &value) { this->add(value); }

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  /*!
   * @copydoc AbstractList::add()
   * @see add()
   * @note If the list is mutable and the values saved in the list are not primitives, nothing happen.
   */
  void operator+(T &&value) { this->add(value); }
#endif

  /*!
   * @copydoc AbstractList::addAll(AbstractList<T>&)
   * @note  If the other list is mutable and the values saved in the lists are primitives, nothing happens.
   * @see addAll(AbstractList<T>&)
   */
  void operator+(AbstractList<T> &list) { this->addAll(list); }
};

#endif// LIST_ABSTRACT_LIST_HPP
