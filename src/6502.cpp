/*

MIT License

Copyright (c) 2016-2017 Mark Allender


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

//#define FUNCTIONAL_TESTS

/*
*  Source file for 6502 emulation layer
*/
#include "apple2emu_defs.h"
#include "string.h"
#include "debugger.h"

#include "6502.h"
#include "memory.h"

cpu_6502::opcode_info cpu_6502::m_6502_opcodes[] = {
 // 0x00 - 0x0f
 { 'BRK ', 1, 7, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ORA ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'TSB ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'ORA ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ASL ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'PHP ', 1, 3, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ORA ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'ASL ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'TSB ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ORA ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ASL ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x10 -
 { 'BPL ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'ORA ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'ORA ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'ASL ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CLC ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ORA ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'ORA ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'ASL ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x20 -
 { 'JSR ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'AND ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'BIT ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'AND ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ROL ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'PLP ', 1, 4, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'AND ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'ROL ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'BIT ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'AND ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ROL ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },


 // 0x30 -
 { 'BMI ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'AND ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'AND ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'ROL ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'SEC ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'AND ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'AND ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'ROL ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x40 -
 { 'RTI ', 1, 6, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'EOR ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'EOR ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'LSR ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode  },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'PHA ', 1, 3, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'EOR ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'LSR ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'JMP ', 3, 3, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'EOR ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'LSR ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x50 -
 { 'BVC ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'EOR ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'EOR ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'LSR ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CLI ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'EOR ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'EOR ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'LSR ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x60 -
 { 'RTS ', 1, 6, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ADC ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'ADC ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ROR ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'PLA ', 1, 4, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ADC ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'ROR ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'JMP ', 3, 5, addr_mode::INDIRECT_MODE, &cpu_6502::indirect_mode },
 { 'ADC ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ROR ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x70 -
 { 'BVS ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'ADC ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'ADC ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'ROR ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'SEI ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ADC ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'ADC ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'ROR ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x80 -
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'STA ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'STY ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'STA ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'STX ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'DEY ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'TXA ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'STY ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'STA ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'STX ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0x90 -
 { 'BCC ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'STA ', 2, 6, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'STY ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'STA ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'STX ', 2, 4, addr_mode::ZP_INDEXED_MODE_Y, &cpu_6502::zero_page_indexed_mode_y },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'TYA ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STA ', 3, 5, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_mode },
 { 'TXS ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'STA ', 3, 5, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0xa0 -
 { 'LDY ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'LDA ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'LDX ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'LDY ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'LDA ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'LDX ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'TAY ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDA ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'TAX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'LDY ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'LDA ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'LDX ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0xb0 -
 { 'BCS ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'LDA ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'LDY ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'LDA ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'LDX ', 2, 4, addr_mode::ZP_INDEXED_MODE_Y, &cpu_6502::zero_page_indexed_mode_y },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CLV ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDA ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'TSX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'LDY ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'LDA ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'LDX ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0xc0 -
 { 'CPY ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'CMP ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CPY ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'CMP ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'DEC ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'INY ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CMP ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'DEX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CPY ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'CMP ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'DEC ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0xd0 -
 { 'BNE ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'CMP ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CMP ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'DEC ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CLD ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CMP ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CMP ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'DEC ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0xe0 -
 { 'CPX ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'SBC ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CPX ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'SBC ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'INC ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'INX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'SBC ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'CPX ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'SBC ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'INC ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },

 // 0xf0 -
 { 'BEQ ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'SBC ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'SBC ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'INC ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'SED ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'SBC ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
 { 'SBC ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'INC ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { '    ', 1, 0, addr_mode::NO_MODE, nullptr },
};

////////////////
// 65c02 opcodes
////////////////
cpu_6502::opcode_info cpu_6502::m_65c02_opcodes[] = {
 // 0x00 - 0x0f
 { 'BRK ', 1, 7, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ORA ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'NOP ', 2, 2, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'TSB ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ORA ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ASL ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'PHP ', 1, 3, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ORA ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'ASL ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'TSB ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ORA ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ASL ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x10 -
 { 'BPL ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'ORA ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { 'ORA ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'TRB ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ORA ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'ASL ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CLC ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ORA ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'INC ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'TRB ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ORA ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'ASL ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x20 -
 { 'JSR ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'AND ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'NOP ', 2, 2, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'BIT ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'AND ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ROL ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'PLP ', 1, 4, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'AND ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'ROL ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'BIT ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'AND ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ROL ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },


 // 0x30 -
 { 'BMI ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'AND ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { 'AND ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'BIT ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'AND ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'ROL ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'SEC ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'AND ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'DEC ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'BIT ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'AND ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'ROL ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x40 -
 { 'RTI ', 1, 6, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'EOR ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'NOP ', 2, 2, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 2, 3, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'EOR ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'LSR ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode  },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'PHA ', 1, 3, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'EOR ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'LSR ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'JMP ', 3, 3, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'EOR ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'LSR ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x50 -
 { 'BVC ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'EOR ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { 'EOR ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 2, 4, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'EOR ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'LSR ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CLI ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'EOR ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'PHY ', 1, 3, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 3, 8, addr_mode::IMPLIED_MODE, &cpu_6502::absolute_mode },
 { 'EOR ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'LSR ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x60 -
 { 'RTS ', 1, 6, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ADC ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'NOP ', 2, 2, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STZ ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ADC ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'ROR ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'PLA ', 1, 4, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ADC ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'ROR ', 1, 2, addr_mode::ACCUMULATOR_MODE, &cpu_6502::accumulator_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'JMP ', 3, 5, addr_mode::INDIRECT_MODE, &cpu_6502::indirect_mode },
 { 'ADC ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'ROR ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x70 -
 { 'BVS ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'ADC ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { 'ADC ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect},
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STZ ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'ADC ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'ROR ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'SEI ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'ADC ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'PLY ', 1, 4, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'JMP ', 3, 6, addr_mode::ABSOLUTE_INDEXED_INDIRECT_MODE, &cpu_6502::absolute_indexed_indirect_mode},
 { 'ADC ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'ROR ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x80 -
 { 'BRA ', 2, 3, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'STA ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'NOP ', 2, 2, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STY ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'STA ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'STX ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'DEY ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'BIT ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'TXA ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STY ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'STA ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'STX ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0x90 -
 { 'BCC ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'STA ', 2, 6, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_mode },
 { 'STA ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STY ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'STA ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'STX ', 2, 4, addr_mode::ZP_INDEXED_MODE_Y, &cpu_6502::zero_page_indexed_mode_y },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'TYA ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STA ', 3, 5, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_mode },
 { 'TXS ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'STZ ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'STA ', 3, 5, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'STZ ', 3, 5, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0xa0 -
 { 'LDY ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'LDA ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'LDX ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDY ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'LDA ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'LDX ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'TAY ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDA ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'TAX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDY ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'LDA ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'LDX ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0xb0 -
 { 'BCS ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'LDA ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { 'LDA ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDY ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'LDA ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'LDX ', 2, 4, addr_mode::ZP_INDEXED_MODE_Y, &cpu_6502::zero_page_indexed_mode_y },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CLV ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDA ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'TSX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'LDY ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'LDA ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'LDX ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0xc0 -
 { 'CPY ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'CMP ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'NOP ', 2, 2, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CPY ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'CMP ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'DEC ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'INY ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CMP ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'DEX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CPY ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'CMP ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'DEC ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0xd0 -
 { 'BNE ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'CMP ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { 'CMP ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 2, 4, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'CMP ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'DEC ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CLD ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CMP ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'PHX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 3, 4, addr_mode::IMPLIED_MODE, &cpu_6502::absolute_mode },
 { 'CMP ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'DEC ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0xe0 -
 { 'CPX ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'SBC ', 2, 6, addr_mode::INDEXED_INDIRECT_MODE, &cpu_6502::indexed_indirect_mode },
 { 'NOP ', 2, 2, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CPX ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'SBC ', 2, 3, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'INC ', 2, 5, addr_mode::ZERO_PAGE_MODE, &cpu_6502::zero_page_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'INX ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'SBC ', 2, 2, addr_mode::IMMEDIATE_MODE, &cpu_6502::immediate_mode },
 { 'NOP ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'CPX ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'SBC ', 3, 4, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'INC ', 3, 6, addr_mode::ABSOLUTE_MODE, &cpu_6502::absolute_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },

 // 0xf0 -
 { 'BEQ ', 2, 2, addr_mode::RELATIVE_MODE, &cpu_6502::relative_mode },
 { 'SBC ', 2, 5, addr_mode::INDIRECT_INDEXED_MODE, &cpu_6502::indirect_indexed_check_boundary_mode },
 { 'SBC ', 2, 5, addr_mode::INDIRECT_ZP_MODE, &cpu_6502::zero_page_indirect },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 2, 4, addr_mode::IMPLIED_MODE, &cpu_6502::immediate_mode },
 { 'SBC ', 2, 4, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'INC ', 2, 6, addr_mode::ZP_INDEXED_MODE, &cpu_6502::zero_page_indexed_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'SED ', 1, 2, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'SBC ', 3, 4, addr_mode::Y_INDEXED_MODE, &cpu_6502::absolute_y_check_boundary_mode },
 { 'PLX ', 1, 4, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
 { 'NOP ', 3, 4, addr_mode::IMPLIED_MODE, &cpu_6502::absolute_mode },
 { 'SBC ', 3, 4, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_check_boundary_mode },
 { 'INC ', 3, 7, addr_mode::X_INDEXED_MODE, &cpu_6502::absolute_x_mode },
 { 'NOP ', 1, 1, addr_mode::IMPLIED_MODE, &cpu_6502::implied_mode },
};


void cpu_6502::init(cpu_6502::cpu_mode mode)
{
	m_pc = 0;
	m_sp = 0xff - 3;     // reset pulls 3 values from the stack
	m_xindex = 0xff;
	m_yindex = 0xff;
	m_acc = 0xff;
	m_status_register = 0xff;
	set_flag(register_bit::DECIMAL_BIT, 0);
	set_flag(register_bit::NOT_USED_BIT, 1);

	// set the opcodes based on what cpu we are emulating
	if (mode == cpu_6502::cpu_mode::CPU_6502) {
		m_opcodes = m_6502_opcodes;
	} else {
		m_opcodes = m_65c02_opcodes;
	}

	// start vector
	set_pc(memory_read(0xfffc) | memory_read(0xfffd) << 8);
}

cpu_6502::opcode_info *cpu_6502::get_opcode(uint8_t val)
{
	return &m_opcodes[val];
}

inline int16_t cpu_6502::accumulator_mode()
{
	return m_acc;
}

inline int16_t cpu_6502::immediate_mode()
{
	return m_pc++;
}

inline int16_t cpu_6502::implied_mode()
{
	return 0;  // this value won't get used
}

inline int16_t cpu_6502::relative_mode()
{
	return m_pc++;
}

inline int16_t cpu_6502::absolute_mode()
{
	uint8_t lo = memory_read(m_pc++);
	uint8_t hi = memory_read(m_pc++);
	return (hi << 8) | lo;
}

inline int16_t cpu_6502::zero_page_mode()
{
	uint8_t lo = memory_read(m_pc++);
	return lo;
}

inline int16_t cpu_6502::indirect_mode()
{
	uint8_t addr_lo = memory_read(m_pc++);
	uint8_t addr_hi = memory_read(m_pc++);
	uint16_t addr = addr_hi << 8 | addr_lo;
	uint16_t return_addr = memory_read(addr) & 0xff;
	return_addr |= (memory_read(addr + 1) << 8);
	return return_addr;
}

inline int16_t cpu_6502::absolute_x_mode()
{
	uint8_t lo = memory_read(m_pc++);
	uint8_t hi = memory_read(m_pc++);
	return ((hi << 8) | lo) + m_xindex;
}

inline int16_t cpu_6502::absolute_y_mode()
{
	uint8_t lo = memory_read(m_pc++);
	uint8_t hi = memory_read(m_pc++);
	return ((hi << 8) | lo) + m_yindex;
}

inline int16_t cpu_6502::absolute_x_check_boundary_mode()
{
	// get the new address
	uint8_t lo = memory_read(m_pc++);
	uint8_t hi = memory_read(m_pc++);
	uint16_t addr = ((hi << 8) | lo) + m_xindex;
	if (((addr - m_xindex) ^ addr) >> 8) {
		// we have crossed page boundary, so cycle count increases
		// by one here
		m_extra_cycles += 1;
	}

	return addr;
}

inline int16_t cpu_6502::absolute_y_check_boundary_mode()
{
	// get the new address
	uint8_t lo = memory_read(m_pc++);
	uint8_t hi = memory_read(m_pc++);
	uint16_t addr = ((hi << 8) | lo) + m_yindex;
	if (((addr - m_yindex) ^ addr) >> 8) {
		// we have crossed page boundary, so cycle count increases
		// by one here
		m_extra_cycles += 1;
	}

	return addr;
}

inline int16_t cpu_6502::zero_page_indexed_mode()
{
	uint8_t lo = memory_read(m_pc++);
	return (lo + m_xindex) & 0xff;
}

inline int16_t cpu_6502::zero_page_indexed_mode_y()
{
	uint8_t lo = memory_read(m_pc++);
	return (lo + m_yindex) & 0xff;
}

inline int16_t cpu_6502::zero_page_indirect()
{
	uint16_t addr_start = memory_read(m_pc++);
	uint8_t lo = memory_read(addr_start);
	uint8_t hi = memory_read(addr_start + 1);
	return (hi << 8) | lo;
}

inline int16_t cpu_6502::indexed_indirect_mode()
{
	uint16_t addr_start = (memory_read(m_pc++) + m_xindex) & 0xff;
	uint8_t lo = memory_read(addr_start);
	uint8_t hi = memory_read(addr_start + 1);
	return (hi << 8) | lo;
}

inline int16_t cpu_6502::absolute_indexed_indirect_mode()
{
	uint8_t lo = memory_read(m_pc++);
	uint8_t hi = memory_read(m_pc++);
	uint16_t addr = ((hi << 8) | lo) + m_xindex;
	lo = memory_read(addr);
	hi = memory_read(addr + 1);
	return (hi << 8) | lo;
}

inline int16_t cpu_6502::indirect_indexed_mode()
{
	uint16_t addr_start = memory_read(m_pc++);
	uint8_t lo = memory_read(addr_start);
	uint8_t hi = memory_read(addr_start + 1);
	return ((hi << 8) | lo) + m_yindex;
}

inline int16_t cpu_6502::indirect_indexed_check_boundary_mode()
{
	uint16_t addr = memory_read(m_pc++);
	uint8_t lo = memory_read(addr);
	uint8_t hi = memory_read(addr+ 1);
	addr = ((hi << 8) | lo) + m_yindex;

	// add extra cycle if page boundary crossed
	if (((addr - m_yindex) ^ addr) >> 8) {
		m_extra_cycles++;
	}
	return addr;
}

inline void cpu_6502::branch_relative()
{
	uint16_t save_pc = m_pc;
	uint8_t val = memory_read(m_pc - 1);
	m_pc += val;
	if (val & 0x80) {
		m_pc -= 0x100;
	}

	// one extra cycle when crossing page boundary
	if ((save_pc ^ m_pc) >> 8) {
		m_extra_cycles++;
	}
	// always extra cycle for branching
	m_extra_cycles++;
}

//
// main loop for opcode processing
//
uint32_t cpu_6502::process_opcode()
{
	m_extra_cycles = 0;

	// get the opcode at the program counter and the just figure out what
	// to do
	uint8_t opcode = memory_read(m_pc++, true);

	// get addressing mode and then do appropriate work based on the mode
	addr_mode mode = m_opcodes[opcode].m_addr_mode;
	SDL_assert(mode != addr_mode::NO_MODE);
	SDL_assert(m_opcodes[opcode].m_cycle_count != 0);
	SDL_assert(m_opcodes[opcode].m_addr_func != nullptr);
	uint16_t src = (this->*m_opcodes[opcode].m_addr_func)();

	uint8_t cycles = m_opcodes[opcode].m_cycle_count;

	// execute the actual opcode
	switch (m_opcodes[opcode].m_mnemonic) {
	case 'ADC ':
	{
		uint8_t val = memory_read(src);
		uint32_t carry_bit = get_flag(register_bit::CARRY_BIT);
		uint32_t sum = (uint32_t)m_acc + (uint32_t)val + carry_bit;
		if (get_flag(register_bit::DECIMAL_BIT)) {
			// decimal mode - see http://www.6502.org/tutorials/decimal_mode.html#3.2.2
			// for detailed information on BCD and flags.  One of the
			// most trickieest parts of the opcode set

			// set the zero bit flag for 6502 as it is based
			// on the binary sum as done before the consitionals
			set_flag(register_bit::ZERO_BIT, ((sum & 0xff) == 0));
			set_flag(register_bit::OVERFLOW_BIT, ~(m_acc ^ val) & (m_acc ^ sum) & 0x80);

			int32_t al = (m_acc & 0xf) + (val & 0xf) + carry_bit;
			if (al >= 0x0a) {
				al = ((al + 0x06) & 0x0f) + 0x10;
			}
			sum = (m_acc & 0xf0) + (val & 0xf0) + al;
			set_flag(register_bit::SIGN_BIT, (sum & 0x80));

			if (sum >= 0xa0) {
				sum += 0x60;
			}
			set_flag(register_bit::CARRY_BIT, (sum >= 0x100));

			m_acc = sum & 0xff;

			// sign bit is different on 6502 and 65c02
			if (m_opcodes == m_65c02_opcodes) {
				set_flag(register_bit::ZERO_BIT, ((sum & 0xff) == 0));
				set_flag(register_bit::SIGN_BIT, (m_acc & 0x80));
				set_flag(register_bit::OVERFLOW_BIT, (sum >= 128));
			}
		} else {
			set_flag(register_bit::CARRY_BIT, sum > 0xff);
			set_flag(register_bit::OVERFLOW_BIT, ~(m_acc ^ val) & (m_acc ^ sum) & 0x80);
			m_acc = sum & 0xff;
			set_flag(register_bit::ZERO_BIT, (m_acc == 0));
			set_flag(register_bit::SIGN_BIT, (m_acc & 0x80));
		}
		break;
	}

	case 'AND ':
	{
		m_acc &= memory_read(src);
		set_flag(register_bit::SIGN_BIT, (m_acc >> 7));
		set_flag(register_bit::ZERO_BIT, (m_acc == 0));
		break;
	}

	case 'ASL ':
	{
		uint8_t val, old_val;
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			old_val = m_acc;
		}
		else {
			old_val = memory_read(src);
		}
		val = old_val << 1;
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			m_acc = val;
		}
		else {
			memory_write(src, val);
		}
		set_flag(register_bit::SIGN_BIT, val >> 7);
		set_flag(register_bit::ZERO_BIT, val == 0);
		set_flag(register_bit::CARRY_BIT, old_val >> 7);
		break;
	}

	case 'BCC ':
	{
		if (get_flag(register_bit::CARRY_BIT) == 0) {
			branch_relative();
		}
		break;
	}
	case 'BCS ':
	{
		if (get_flag(register_bit::CARRY_BIT) == 1) {
			branch_relative();
		}
		break;
	}
	case 'BEQ ':
	{
		if (get_flag(register_bit::ZERO_BIT) == 1) {
			branch_relative();
		}
		break;
	}
	case 'BIT ':
	{
		int8_t val = memory_read(src);
		set_flag(register_bit::ZERO_BIT, ((val & m_acc) == 0));

		// immediate mode with BIT only sets the zero flag
		if (mode != cpu_6502::addr_mode::IMMEDIATE_MODE) {
			set_flag(register_bit::SIGN_BIT, ((val >> 7) & 0x1));
			set_flag(register_bit::OVERFLOW_BIT, ((val >> 6) & 0x1));
		}
		break;
	}
	case 'BMI ':
	{
		if (get_flag(register_bit::SIGN_BIT) == 1) {
			branch_relative();
		}
		break;
	}
	case 'BNE ':
	{
		if (get_flag(register_bit::ZERO_BIT) == 0) {
			branch_relative();
		}
		break;
	}
	case 'BPL ':
	{
		if (get_flag(register_bit::SIGN_BIT) == 0) {
			branch_relative();
		}
		break;
	}
	case 'BRA ':
	{
		branch_relative();
		break;
	}
	case 'BRK ':
	{
		m_pc++;
		memory_write(0x100 + m_sp--, (m_pc >> 8));
		memory_write(0x100 + m_sp--, (m_pc & 0xff));
		uint8_t register_value = m_status_register;
		register_value |= (1 << static_cast<uint8_t>(register_bit::NOT_USED_BIT));
		register_value |= (1 << static_cast<uint8_t>(register_bit::BREAK_BIT));
		memory_write(0x100 + m_sp--, register_value);
		set_flag(register_bit::INTERRUPT_BIT, 1);
		if (m_opcodes == m_65c02_opcodes) {
			set_flag(register_bit::DECIMAL_BIT, 0);
		}

		// clear decimal flag on break
		m_pc = (memory_read(0xfffe) & 0xff) | (memory_read(0xffff) << 8);
		break;
	}
	case 'BVC ':
	{
		if (get_flag(register_bit::OVERFLOW_BIT) == 0) {
			branch_relative();
		}
		break;
	}
	case 'BVS ':
	{
		if (get_flag(register_bit::OVERFLOW_BIT) == 1) {
			branch_relative();
		}
		break;
	}
	case 'CLC ':
	{
		set_flag(register_bit::CARRY_BIT, 0);
		break;
	}
	case 'CLD ':
	{
		set_flag(register_bit::DECIMAL_BIT, 0);
		break;
	}
	case 'CLI ':
	{
		set_flag(register_bit::INTERRUPT_BIT, 0);
		break;
	}
	case 'CLV ':
	{
		set_flag(register_bit::OVERFLOW_BIT, 0);
		break;
	}
	case 'CMP ':
	{
		uint8_t src_val = memory_read(src);
		if (m_acc >= src_val) {
			set_flag(register_bit::CARRY_BIT, 1);
		}
		else {
			set_flag(register_bit::CARRY_BIT, 0);
		}
		int8_t val = m_acc - src_val;
		set_flag(register_bit::SIGN_BIT, (val >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, val == 0);
		break;
	}
	case 'CPX ':
	{
		uint8_t src_val = memory_read(src);
		if (m_xindex >= src_val) {
			set_flag(register_bit::CARRY_BIT, 1);
		}
		else {
			set_flag(register_bit::CARRY_BIT, 0);
		}
		int8_t val = m_xindex - src_val;
		set_flag(register_bit::SIGN_BIT, (val >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, val == 0);
		break;
	}
	case 'CPY ':
	{
		uint8_t src_val = memory_read(src);
		if (m_yindex >= src_val) {
			set_flag(register_bit::CARRY_BIT, 1);
		}
		else {
			set_flag(register_bit::CARRY_BIT, 0);
		}
		int8_t val = m_yindex - src_val;
		set_flag(register_bit::SIGN_BIT, (val >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, val == 0);
		break;
	}
	case 'DEC ':
	{
		uint8_t val;
		if (mode == cpu_6502::addr_mode::ACCUMULATOR_MODE) {
			m_acc--;
			val = m_acc;
		} else {
			val = memory_read(src) - 1;
			memory_write(src, val);
		}
		set_flag(register_bit::SIGN_BIT, (val >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, val == 0);
		break;
	}
	case 'DEX ':
	{
		m_xindex--;
		set_flag(register_bit::SIGN_BIT, (m_xindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_xindex == 0) & 0x1);
		break;
	}
	case 'DEY ':
	{
		m_yindex--;
		set_flag(register_bit::SIGN_BIT, (m_yindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_yindex == 0) & 0x1);
		break;
	}
	case 'EOR ':
	{
		uint8_t val = memory_read(src);
		m_acc ^= val;
		set_flag(register_bit::SIGN_BIT, (m_acc >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_acc == 0) & 0x1);
		break;
	}
	case 'INC ':
	{
		uint8_t val;
		if (mode == cpu_6502::addr_mode::ACCUMULATOR_MODE) {
			m_acc++;
			val = m_acc;
		} else {
			val = memory_read(src) + 1;
			memory_write(src, val);
		}
		set_flag(register_bit::SIGN_BIT, (val >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, val == 0);
		break;
	}
	break;
	case 'INX ':
	{
		m_xindex++;
		set_flag(register_bit::SIGN_BIT, (m_xindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_xindex == 0) & 0x1);
		break;
	}
	case 'INY ':
	{
		m_yindex++;
		set_flag(register_bit::SIGN_BIT, (m_yindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_yindex == 0) & 0x1);
		break;
	}
	case 'JMP ':
	{
		m_pc = src;
		break;
	}
	case 'JSR ':
	{
#if defined(FUNCTIONAL_TESTS)
		if (src == 0xf001) {
			debugger_print_char_to_console(m_acc);
		}
		else if (src == 0xf004) {
//			m_acc = _getch();
//		} else if (src == 0x43bf) {
			//debug_break();
		} else {
			memory_write(0x100 + m_sp--, ((m_pc - 1) >> 8));
			memory_write(0x100 + m_sp--, (m_pc - 1) & 0xff);
			m_pc = src;
		}
#else
		memory_write(0x100 + m_sp--, ((m_pc - 1) >> 8));
		memory_write(0x100 + m_sp--, (m_pc - 1) & 0xff);
		m_pc = src;
#endif
		break;
	}
	case 'LDA ':
	{
		m_acc = memory_read(src);
		set_flag(register_bit::SIGN_BIT, (m_acc >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_acc == 0) & 0x1);
		break;
	}
	case 'LDX ':
	{
		m_xindex = memory_read(src);
		set_flag(register_bit::SIGN_BIT, (m_xindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_xindex == 0) & 0x1);
		break;
	}
	case 'LDY ':
	{
		m_yindex = memory_read(src);
		set_flag(register_bit::SIGN_BIT, (m_yindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, (m_yindex == 0) & 0x1);
		break;
	}
	case 'LSR ':
	{
		uint8_t val;
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			val = m_acc;
		}
		else {
			val = memory_read(src);
		}
		set_flag(register_bit::CARRY_BIT, val & 1);
		val >>= 1;
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			m_acc = val;
		}
		else {
			memory_write(src, val);
		}
		set_flag(register_bit::SIGN_BIT, 0);
		set_flag(register_bit::ZERO_BIT, val == 0);
		break;
	}
	case 'NOP ':
	{
		break;
	}
	case 'ORA ':
	{
		uint8_t val = memory_read(src);
		m_acc |= val;
		set_flag(register_bit::SIGN_BIT, (m_acc >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, m_acc == 0);
		break;
	}
	case 'PHA ':
	{
		memory_write(0x100 + m_sp--, m_acc);
		break;
	}
	case 'PHX ':
	{
		memory_write(0x100 + m_sp--, m_xindex);
		break;
	}
	case 'PHY ':
	{
		memory_write(0x100 + m_sp--, m_yindex);
		break;
	}
	case 'PLA ':
	{
		m_acc = memory_read(0x100 + ++m_sp);
		set_flag(register_bit::SIGN_BIT, (m_acc >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, m_acc == 0);
		break;
	}
	case 'PLX ':
	{
		m_xindex = memory_read(0x100 + ++m_sp);
		set_flag(register_bit::SIGN_BIT, (m_xindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, m_xindex == 0);
		break;
	}
	case 'PLY ':
	{
		m_yindex = memory_read(0x100 + ++m_sp);
		set_flag(register_bit::SIGN_BIT, (m_yindex >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, m_yindex == 0);
		break;
	}
	case 'PHP ':
	{
		uint8_t register_value = m_status_register;
		register_value |= 1 << (static_cast<uint8_t>(register_bit::BREAK_BIT));
		register_value |= 1 << (static_cast<uint8_t>(register_bit::NOT_USED_BIT));
		memory_write(0x100 + m_sp--, register_value);
		break;
	}
	case 'PLP ':
	{
		m_status_register = memory_read(0x100 + ++m_sp);
		break;
	}
	case 'ROL ':
	{
		uint8_t val;
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			val = m_acc;
		}
		else {
			val = memory_read(src);
		}
		uint8_t carry_bit = (val >> 7) & 0x1;
		val <<= 1;
		val |= get_flag(register_bit::CARRY_BIT);
		set_flag(register_bit::CARRY_BIT, carry_bit);
		set_flag(register_bit::SIGN_BIT, (val >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, val == 0);
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			m_acc = val;
		}
		else {
			memory_write(src, val);
		}
		break;
	}
	case 'ROR ':
	{
		uint8_t val;
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			val = m_acc;
		}
		else {
			val = memory_read(src);
		}
		uint8_t carry_bit = val & 0x1;
		val >>= 1;
		val |= (get_flag(register_bit::CARRY_BIT) & 0x1) << 7;
		set_flag(register_bit::CARRY_BIT, carry_bit);
		set_flag(register_bit::SIGN_BIT, (val >> 7) & 0x1);
		set_flag(register_bit::ZERO_BIT, val == 0);
		if (mode == addr_mode::ACCUMULATOR_MODE) {
			m_acc = val;
		}
		else {
			memory_write(src, val);
		}
		break;
	}
	case 'RTI ':
	{
		m_status_register = memory_read(0x100 + ++m_sp);
		m_pc = memory_read(0x100 + ++m_sp);
		m_pc = (memory_read(0x100 + ++m_sp) << 8) | m_pc;
		break;
	}
	case 'RTS ':
	{
		uint16_t addr = memory_read(0x100 + ++m_sp) & 0x00ff;
		addr |= (memory_read(0x100 + ++m_sp) << 8);
		m_pc = addr;
		m_pc++;
		break;
	}
	case 'SBC ':
	{
		uint8_t val = memory_read(src);
		int32_t sum;
		uint32_t carry_bit = get_flag(register_bit::CARRY_BIT);
		sum = m_acc + (~val & 0xff) + carry_bit;
		if (get_flag(register_bit::DECIMAL_BIT)) {
			// decimal mode.

			// behavior of ADC is slightly different on 6502 and 65c02.
			// just split it out into separate conditionals to make code
			// easier to understand
			if (m_opcodes == m_6502_opcodes) {
				set_flag(register_bit::CARRY_BIT, sum > 0xff);
				set_flag(register_bit::OVERFLOW_BIT, (m_acc ^ val) & (m_acc ^ sum) & 0x80);
				int32_t al = (m_acc & 0x0f) - (val & 0x0f) + carry_bit - 1;
				if (al < 0) {
					al = ((al - 0x06) & 0x0f) - 0x10;
				}
				sum = (m_acc & 0xf0) - (val & 0xf0) + al;
				if (sum < 0) {
					sum = sum - 0x60;
				}
				m_acc = sum & 0xff;

				// flags are set as they are in binary arithmetic mode
				set_flag(register_bit::ZERO_BIT, (m_acc == 0));
				set_flag(register_bit::SIGN_BIT, (m_acc & 0x80));
			} else {
				set_flag(register_bit::CARRY_BIT, sum > 0xff);
				set_flag(register_bit::OVERFLOW_BIT, (m_acc ^ val) & (m_acc ^ sum) & 0x80);
				int32_t al = (m_acc & 0x0f) - (val & 0x0f) + carry_bit - 1;
				sum = m_acc - val + carry_bit - 1;
				if (sum < 0) {
					sum = sum - 0x60;
				}
				if (al < 0) {
					sum = sum - 0x06;
				}
				m_acc = sum & 0xff;

				// flags are set as they are in binary arithmetic mode
				set_flag(register_bit::ZERO_BIT, (m_acc == 0));
				set_flag(register_bit::SIGN_BIT, (m_acc & 0x80));
			}
		} else {
			set_flag(register_bit::CARRY_BIT, sum > 0xff);
			set_flag(register_bit::OVERFLOW_BIT, (m_acc ^ val) & (m_acc ^ sum) & 0x80);
			m_acc = sum & 0xff;
			set_flag(register_bit::ZERO_BIT, (m_acc == 0));
			set_flag(register_bit::SIGN_BIT, (m_acc & 0x80));
		}
		break;
	}
	case 'SEC ':
	{
		set_flag(register_bit::CARRY_BIT, 1);
		break;
	}
	case 'SED ':
	{
		set_flag(register_bit::DECIMAL_BIT, 1);
		break;
	}
	case 'SEI ':
	{
		set_flag(register_bit::INTERRUPT_BIT, 1);
		break;
	}
	case 'STA ':
	{
		memory_write(src, m_acc);
		break;
	}
	case 'STX ':
	{
		memory_write(src, m_xindex);
		break;
	}
	case 'STY ':
	{
		memory_write(src, m_yindex);
		break;
	}
	case 'STZ ':
	{
		memory_write(src, 0);
		break;
	}
	case 'TAX ':
	{
		m_xindex = m_acc;
		set_flag(register_bit::ZERO_BIT, m_xindex == 0);
		set_flag(register_bit::SIGN_BIT, m_xindex & 0x80);
		break;
	}
	case 'TRB ':
	{
		int8_t val = memory_read(src);
		set_flag(register_bit::ZERO_BIT, ((val & m_acc) == 0));

		// switch bits in accum
		memory_write(src, (~m_acc) & val);
		break;
	}
	case 'TSB ':
	{
		int8_t val = memory_read(src);
		set_flag(register_bit::ZERO_BIT, ((val & m_acc) == 0));

		// switch bits in accum
		memory_write(src, m_acc | val);
		break;
	}
	case 'TXA ':
	{
		m_acc = m_xindex;
		set_flag(register_bit::ZERO_BIT, m_acc == 0);
		set_flag(register_bit::SIGN_BIT, m_acc & 0x80);
		break;
	}
	case 'TAY ':
	{
		m_yindex = m_acc;
		set_flag(register_bit::ZERO_BIT, m_yindex == 0);
		set_flag(register_bit::SIGN_BIT, m_yindex & 0x80);
		break;
	}
	case 'TYA ':
	{
		m_acc = m_yindex;
		set_flag(register_bit::ZERO_BIT, m_acc == 0);
		set_flag(register_bit::SIGN_BIT, m_acc & 0x80);
		break;
	}
	case 'TXS ':
	{
		m_sp = m_xindex;
		break;
	}
	case 'TSX ':
	{
		m_xindex = (int8_t)m_sp;
		set_flag(register_bit::ZERO_BIT, m_xindex == 0);
		set_flag(register_bit::SIGN_BIT, m_xindex & 0x80);
		break;
	}

	default:
	{
		SDL_assert(0);
	}
	}

	// return the number of cycles that we executed
	return cycles + m_extra_cycles;
}

