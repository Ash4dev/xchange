#pragma once

#include "include/Order.hpp"
#include <list>
#include <memory>

// resources:
// https://chatgpt.com/share/6837389e-78f8-800c-a7e6-9e174c98c870
// http://learncpp.com/cpp-tutorial/stdshared_ptr/
// https://www.learncpp.com/cpp-tutorial/introduction-to-smart-pointers-move-semantics/

// order object is big, no sense carrying around -> use pointer
// dumb/raw: explicit memory management (nightmare at current exp level)
// smart pointers: auto ptr deletion on scope exit (memory leak avoid)
// unique smart: single owner of resource (not allow copy around)
// shared smart: multiple owners of resource (copy allowed)

// in our archi: OrderPointer stored in m_info, m_orderList at once
// if unique used, need to move the ptr around using std::move
// as a result, OrderPointer cannot be stored simulataneously in both
// unique ptr issue in use: https://onlinegdb.com/Q69RTH3Ye

// hence, use of shared ptr is a necessity for our use case
// shared_ptr sample usage: https://onlinegdb.com/iNK_JFDd7
using OrderPointer = std::shared_ptr<Order>;

// stl list: doubly linked list
// optimize insertion/deletion (order)
// tradeoff: slower searching(get away w/ order iterator storing)

// alternatives: map <time, orderinfo>
// but multiple different orders can enter at same time
// map<time, list<orderinfo>>? time info repeat but rest fine
using OrderList = std::list<OrderPointer>;
