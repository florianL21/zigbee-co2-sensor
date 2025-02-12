/*!
 * @file SingleLinkedList.hpp
 *
 * This file is part of the List library. It extends the arduino ecosystem with
 * easy-to-use list implementations. They are specially designed and optimized
 * for different purposes.
 *
 * Copyright (C) 2021-2023  Niklas Kaaf
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

#ifndef LIST_SINGLE_LINKED_LIST_HPP
#define LIST_SINGLE_LINKED_LIST_HPP

#include "AbstractList.hpp"

/*!
 * @brief   Implementation of a single-linked list.
 *
 * @tparam T    Data Type of entries, that should be stored in the list.
 */
template<typename T>
class SingleLinkedList : public AbstractList<T> {
  /*!
   * @brief Class representing one entry of the list.
   */
  class Entry : public AbstractList<T>::AbstractEntry {
    Entry *next = nullptr;/// Pointer to the next element of the list

   public:
    /*!
     * @brief   Destructor of an Entry Object.
     */
    ~Entry() { next = nullptr; }

    /*!
     * @brief   Get the next entry of the list.
     *
     * @return  Pointer to the next element.
     */
    Entry *getNext() const { return next; };

    /*!
     * @brief   Set the next entry of the list.
     *
     * @param nextEntry Pointer to the next entry.
     */
    void setNext(Entry *nextEntry) { next = nextEntry; }
  };

  Entry *head = nullptr;/// The first entry of the list.
  Entry *tail = nullptr;/// The last entry of the list.

 protected:
  /*!
   * @copydoc AbstractList::getPointer()
   */
  T *getPointer(int index) override {
    if (this->isIndexOutOfBounds(index)) {
      return nullptr;
    }

    Entry *current = head;
    int i = 0;
    while (i != index) {
      current = current->getNext();
      i++;
    }
    return current->getValue(this->isMutable());
  }

 public:
  /*!
   * @brief Constructor of a SingleLinkedList Object.
   *
   * @param mutableList true if the list should be mutable; false otherwise
   *                    (default).
   */
  explicit SingleLinkedList<T>(bool mutableList = false)
      : AbstractList<T>(mutableList) {}

  /*!
   * @brief Destructor of a SingleLinkedList Object.
   */
  ~SingleLinkedList() { this->clear(); }

  using AbstractList<T>::addAtIndex;///'Using' the addAtIndex method, to
                                    /// prevent name hiding of the addAtIndex
                                    /// method from AbstractList

  /*!
   * @copydoc AbstractList::addAtIndex()
   */
  void addAtIndex(int index, T &value) override {
    // it is allowed, that index == this->getSize() to insert it behind the last
    // entry
    if (extendedIsIndexOutOfBounds(index)) {
      return;
    }

    Entry *entry;

    entry = new Entry();
    entry->setValue(value, this->isMutable());

    if (index == 0) {
      if (this->getSize() == 0) {
        // Add entry to an empty list
        tail = entry;
      } else {
        // Add entry to a not empty list
        entry->setNext(head);
      }
      head = entry;
    } else if (index == this->getSize()) {
      // Add entry at not empty list but at last position
      tail->setNext(entry);
      tail = entry;
    } else {
      // Add entry to not empty list, somewhere in the middle
      Entry *current = head;
      for (int i = 0; i < index - 1; ++i) {
        current = current->getNext();
      }
      entry->setNext(current->getNext());
      current->setNext(entry);
    }

    this->increaseSize();
  };

  /*!
   * @copydoc AbstractList::clear()
   */
  void clear() override {
    if (this->getSize() == 0) {
      return;
    }

    Entry *current = head;
    Entry *next;
    for (int i = 0; i < this->getSize(); ++i) {
      next = current->getNext();

      delete current;
      current = next;
    }

    this->resetSize();
    head = nullptr;
    tail = nullptr;
  }

  /*!
   * @copydoc AbstractList::remove()
   */
  void remove(int index) override {
    if (this->isIndexOutOfBounds(index)) {
      return;
    }

    Entry *current = head;
    // current is either the element to delete if index == 0, or the previous
    // element
    if (index != 0) {
      int i = 0;
      while (i < index - 1) {
        current = current->getNext();
        i++;
      }
    }

    if (index == this->getSize() - 1) {
      tail = current;
    }

    Entry *toDelete;
    if (index == 0) {
      toDelete = current;
      head = current->getNext();
    } else {
      toDelete = current->getNext();
      current->setNext(current->getNext()->getNext());
    }

    delete toDelete;

    this->decreaseSize();

    if (this->getSize() == 0) {
      head = nullptr;
      tail = nullptr;
    }
  }
};

#endif// LIST_SINGLE_LINKED_LIST_HPP
