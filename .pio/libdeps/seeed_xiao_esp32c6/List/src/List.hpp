/*!
 * @file List.hpp
 *
 * @mainpage
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for the List Library for the Arduino platform.
 * Because of the independence of Arduino libraries, it could be theoretically
 * used for every C/C++ program.
 * It extends the arduino ecosystem with easy-to-use list implementations. They
 * are specially designed and optimized for different purposes.
 *
 *
 * @section author Author
 *
 * Written by Niklas Kaaf (nkaaf@protonmail.com) with passion and the goal to
 * provide a simple and well implemented basic structure for building great
 * software.
 *
 * @section license License
 *
 * This file is part of the List library.
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

#ifndef LIST_HPP
#define LIST_HPP

#include "SingleLinkedList.hpp"

/*!
 * @brief   A list with the most basic implementation as a single-linked list.
 *
 * @tparam T    Data Type of entries, that should be stored in the list.
 */
template<typename T>
class List : public SingleLinkedList<T> {
 public:
  /*!
   * @brief Constructor of a List Object.
   *
   * @param mutableList true if the list should be mutable; false otherwise
   *                    (default).
   * @note  Mutable lists only work as expected, if the values, that are added, are
   *        only lvalues and you can ensure, that the variables do not go out-of-scope during all operations of the list.
   */
  explicit List<T>(bool mutableList = false)
      : SingleLinkedList<T>(mutableList) {}
};

#endif// LIST_HPP
