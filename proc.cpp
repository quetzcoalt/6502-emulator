#include <bits/stdc++.h>

using namespace std;

bitset<16> address_bus;

/* REGISTERS
 * A = Accumulator
 * P = Processor status
 *  n = Negative
 *  v = Overflow
 *  b = Break
 *  d = Decimal
 *  i = Interrupt disable
 *  z = Zero
 *  c = Carry
 * PC = Program counter
 * S = Stack pointer
 * X = Index
 * Y = Index
 * */

bitset<8> A;
bitset<8> P;
bitset<16> PC;
bitset<8> S;
bitset<8> X;
bitset<8> Y;

int main() {
    cout << A << endl;

    return 0;
}

