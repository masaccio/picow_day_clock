/*
 * MIT License
 * 
 * Copyright (c) 2025 Jon Connell
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "fonts.h"

static const uint8_t Roboto_Medium12_space[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_excl[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_quote[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xA0, 0x00, // # #             
  0xA0, 0x00, // # #             
  0xA0, 0x00, // # #             
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_hash[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x28, 0x00, //   # #           
  0x28, 0x00, //   # #           
  0x7C, 0x00, //  #####          
  0x28, 0x00, //   # #           
  0xFC, 0x00, // ######          
  0x50, 0x00, //  # #            
  0x50, 0x00, //  # #            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_dollar[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x20, 0x00, //   #             
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0x40, 0x00, //  #              
  0x30, 0x00, //   ##            
  0x08, 0x00, //     #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x20, 0x00, //   #             
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_percent[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x60, 0x00, //  ##             
  0x94, 0x00, // #  # #          
  0x68, 0x00, //  ## #           
  0x10, 0x00, //    #            
  0x1E, 0x00, //    ####         
  0x2A, 0x00, //   # # #         
  0x0E, 0x00, //     ###         
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_amp[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0x50, 0x00, //  # #            
  0x60, 0x00, //  ##             
  0x54, 0x00, //  # # #          
  0xCC, 0x00, // ##  ##          
  0x7C, 0x00, //  #####          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_apos[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_lparen[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
};

static const uint8_t Roboto_Medium12_rparen[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x80, 0x00, // #               
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x80, 0x00, // #               
};

static const uint8_t Roboto_Medium12_asterisk[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x20, 0x00, //   #             
  0xF0, 0x00, // ####            
  0x60, 0x00, //  ##             
  0x50, 0x00, //  # #            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_plus[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0xF8, 0x00, // #####           
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_comma[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x80, 0x00, // #               
};

static const uint8_t Roboto_Medium12_minus[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xE0, 0x00, // ###             
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_dot[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_slash[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x10, 0x00, //    #            
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_0[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_1[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x30, 0x00, //   ##            
  0x50, 0x00, //  # #            
  0x10, 0x00, //    #            
  0x10, 0x00, //    #            
  0x10, 0x00, //    #            
  0x10, 0x00, //    #            
  0x10, 0x00, //    #            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_2[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0xC8, 0x00, // ##  #           
  0x08, 0x00, //     #           
  0x10, 0x00, //    #            
  0x20, 0x00, //   #             
  0x60, 0x00, //  ##             
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_3[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0x08, 0x00, //     #           
  0x30, 0x00, //   ##            
  0x08, 0x00, //     #           
  0xC8, 0x00, // ##  #           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_4[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x18, 0x00, //    ##           
  0x38, 0x00, //   ###           
  0x38, 0x00, //   ###           
  0x58, 0x00, //  # ##           
  0xF8, 0x00, // #####           
  0x18, 0x00, //    ##           
  0x18, 0x00, //    ##           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_5[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
  0x48, 0x00, //  #  #           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_6[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x30, 0x00, //   ##            
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_7[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xF8, 0x00, // #####           
  0x08, 0x00, //     #           
  0x10, 0x00, //    #            
  0x10, 0x00, //    #            
  0x30, 0x00, //   ##            
  0x20, 0x00, //   #             
  0x60, 0x00, //  ##             
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_8[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x30, 0x00, //   ##            
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_9[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0xC8, 0x00, // ##  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x18, 0x00, //    ##           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_colon[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_semi[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x80, 0x00, // #               
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_lt[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x10, 0x00, //    #            
  0x60, 0x00, //  ##             
  0x60, 0x00, //  ##             
  0x10, 0x00, //    #            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_eq[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_gt[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x38, 0x00, //   ###           
  0x38, 0x00, //   ###           
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_qmark[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0xD0, 0x00, // ## #            
  0x10, 0x00, //    #            
  0x30, 0x00, //   ##            
  0x20, 0x00, //   #             
  0x00, 0x00, //                 
  0x20, 0x00, //   #             
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_at[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x1E, 0x00, //    ####         
  0x21, 0x00, //   #    #        
  0x4D, 0x00, //  #  ## #        
  0x54, 0x80, //  # # #  #       
  0x94, 0x80, // #  # #  #       
  0x95, 0x00, // #  # # #        
  0x5F, 0x00, //  # #####        
  0x40, 0x00, //  #              
  0x3C, 0x00, //   ####          
};

static const uint8_t Roboto_Medium12_A[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x10, 0x00, //    #            
  0x30, 0x00, //   ##            
  0x28, 0x00, //   # #           
  0x68, 0x00, //  ## #           
  0x7C, 0x00, //  #####          
  0x44, 0x00, //  #   #          
  0xC4, 0x00, // ##   #          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_B[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x4C, 0x00, //  #  ##          
  0x4C, 0x00, //  #  ##          
  0x78, 0x00, //  ####           
  0x4C, 0x00, //  #  ##          
  0x4C, 0x00, //  #  ##          
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_C[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x38, 0x00, //   ###           
  0x44, 0x00, //  #   #          
  0x40, 0x00, //  #              
  0xC0, 0x00, // ##              
  0x40, 0x00, //  #              
  0x44, 0x00, //  #   #          
  0x38, 0x00, //   ###           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_D[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x4C, 0x00, //  #  ##          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x4C, 0x00, //  #  ##          
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_E[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_F[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_G[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x38, 0x00, //   ###           
  0x44, 0x00, //  #   #          
  0x40, 0x00, //  #              
  0x5C, 0x00, //  # ###          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x3C, 0x00, //   ####          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_H[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x7C, 0x00, //  #####          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_I[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_J[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
  0xD8, 0x00, // ## ##           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_K[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x4C, 0x00, //  #  ##          
  0x58, 0x00, //  # ##           
  0x50, 0x00, //  # #            
  0x70, 0x00, //  ###            
  0x58, 0x00, //  # ##           
  0x48, 0x00, //  #  #           
  0x44, 0x00, //  #   #          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_L[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_M[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x63, 0x00, //  ##   ##        
  0x63, 0x00, //  ##   ##        
  0x67, 0x00, //  ##  ###        
  0x55, 0x00, //  # # # #        
  0x55, 0x00, //  # # # #        
  0x59, 0x00, //  # ##  #        
  0x49, 0x00, //  #  #  #        
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_N[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x44, 0x00, //  #   #          
  0x64, 0x00, //  ##  #          
  0x74, 0x00, //  ### #          
  0x54, 0x00, //  # # #          
  0x4C, 0x00, //  #  ##          
  0x4C, 0x00, //  #  ##          
  0x44, 0x00, //  #   #          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_O[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x38, 0x00, //   ###           
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0xC4, 0x00, // ##   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x38, 0x00, //   ###           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_P[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x78, 0x00, //  ####           
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_Q[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x38, 0x00, //   ###           
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0xC4, 0x00, // ##   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x38, 0x00, //   ###           
  0x04, 0x00, //      #          
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_R[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x4C, 0x00, //  #  ##          
  0x4C, 0x00, //  #  ##          
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x44, 0x00, //  #   #          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_S[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x4C, 0x00, //  #  ##          
  0x40, 0x00, //  #              
  0x38, 0x00, //   ###           
  0x0C, 0x00, //     ##          
  0xCC, 0x00, // ##  ##          
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_T[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xFC, 0x00, // ######          
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_U[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x44, 0x00, //  #   #          
  0x4C, 0x00, //  #  ##          
  0x38, 0x00, //   ###           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_V[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xC4, 0x00, // ##   #          
  0x44, 0x00, //  #   #          
  0x48, 0x00, //  #  #           
  0x68, 0x00, //  ## #           
  0x28, 0x00, //   # #           
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_W[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xC9, 0x80, // ##  #  ##       
  0x49, 0x00, //  #  #  #        
  0x55, 0x00, //  # # # #        
  0x55, 0x00, //  # # # #        
  0x57, 0x00, //  # # ###        
  0x26, 0x00, //   #  ##         
  0x22, 0x00, //   #   #         
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_X[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x4C, 0x00, //  #  ##          
  0x68, 0x00, //  ## #           
  0x38, 0x00, //   ###           
  0x30, 0x00, //   ##            
  0x38, 0x00, //   ###           
  0x68, 0x00, //  ## #           
  0xCC, 0x00, // ##  ##          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_Y[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xCC, 0x00, // ##  ##          
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x30, 0x00, //   ##            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_Z[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xFC, 0x00, // ######          
  0x08, 0x00, //     #           
  0x10, 0x00, //    #            
  0x30, 0x00, //   ##            
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
  0xFC, 0x00, // ######          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_lbrack[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x60, 0x00, //  ##             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x60, 0x00, //  ##             
};

static const uint8_t Roboto_Medium12_bslash[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x80, 0x00, // #               
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x60, 0x00, //  ##             
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x10, 0x00, //    #            
  0x10, 0x00, //    #            
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_rbrack[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xC0, 0x00, // ##              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0xC0, 0x00, // ##              
};

static const uint8_t Roboto_Medium12_caret[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x20, 0x00, //   #             
  0x60, 0x00, //  ##             
  0x60, 0x00, //  ##             
  0x90, 0x00, // #  #            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_underscore[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xF0, 0x00, // ####            
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_backtick[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_a[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0xC8, 0x00, // ##  #           
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_b[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_c[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0xC0, 0x00, // ##              
  0x48, 0x00, //  #  #           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_d[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0xC8, 0x00, // ##  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_e[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0xF8, 0x00, // #####           
  0x40, 0x00, //  #              
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_f[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x30, 0x00, //   ##            
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0xE0, 0x00, // ###             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_g[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0xC8, 0x00, // ##  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x08, 0x00, //     #           
  0x70, 0x00, //  ###            
};

static const uint8_t Roboto_Medium12_h[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_i[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_j[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x80, 0x00, // #               
  0x00, 0x00, //                 
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x80, 0x00, // #               
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_k[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x58, 0x00, //  # ##           
  0x70, 0x00, //  ###            
  0x60, 0x00, //  ##             
  0x50, 0x00, //  # #            
  0x48, 0x00, //  #  #           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_l[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_m[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x7F, 0x00, //  #######        
  0x49, 0x00, //  #  #  #        
  0x49, 0x00, //  #  #  #        
  0x49, 0x00, //  #  #  #        
  0x49, 0x00, //  #  #  #        
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_n[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_o[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x48, 0x00, //  #  #           
  0xC8, 0x00, // ##  #           
  0x48, 0x00, //  #  #           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_p[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
};

static const uint8_t Roboto_Medium12_q[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x78, 0x00, //  ####           
  0x48, 0x00, //  #  #           
  0xC8, 0x00, // ##  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x08, 0x00, //     #           
  0x08, 0x00, //     #           
};

static const uint8_t Roboto_Medium12_r[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x60, 0x00, //  ##             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_s[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x70, 0x00, //  ###            
  0x58, 0x00, //  # ##           
  0x70, 0x00, //  ###            
  0xC8, 0x00, // ##  #           
  0x70, 0x00, //  ###            
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_t[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0xE0, 0x00, // ###             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x60, 0x00, //  ##             
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_u[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x48, 0x00, //  #  #           
  0x78, 0x00, //  ####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_v[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x98, 0x00, // #  ##           
  0x50, 0x00, //  # #            
  0x50, 0x00, //  # #            
  0x50, 0x00, //  # #            
  0x20, 0x00, //   #             
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_w[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x92, 0x00, // #  #  #         
  0x5A, 0x00, //  # ## #         
  0x6E, 0x00, //  ## ###         
  0x6C, 0x00, //  ## ##          
  0x64, 0x00, //  ##  #          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_x[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x58, 0x00, //  # ##           
  0x50, 0x00, //  # #            
  0x20, 0x00, //   #             
  0x50, 0x00, //  # #            
  0xD8, 0x00, // ## ##           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_y[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x98, 0x00, // #  ##           
  0x50, 0x00, //  # #            
  0x50, 0x00, //  # #            
  0x70, 0x00, //  ###            
  0x20, 0x00, //   #             
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
};

static const uint8_t Roboto_Medium12_z[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0xF8, 0x00, // #####           
  0x10, 0x00, //    #            
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
  0xF8, 0x00, // #####           
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_lbrace[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0xC0, 0x00, // ##              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x20, 0x00, //   #             
};

static const uint8_t Roboto_Medium12_pipe[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x00, 0x00, //                 
};

static const uint8_t Roboto_Medium12_rbrace[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x80, 0x00, // #               
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x20, 0x00, //   #             
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x40, 0x00, //  #              
  0x80, 0x00, // #               
};

static const uint8_t Roboto_Medium12_tilde[] = {
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x64, 0x00, //  ##  #          
  0x5C, 0x00, //  # ###          
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
};

static const vFONTENTRY Roboto_Medium12_Table[] = {
  { 2, Roboto_Medium12_space },
  { 3, Roboto_Medium12_excl },
  { 3, Roboto_Medium12_quote },
  { 6, Roboto_Medium12_hash },
  { 6, Roboto_Medium12_dollar },
  { 7, Roboto_Medium12_percent },
  { 7, Roboto_Medium12_amp },
  { 2, Roboto_Medium12_apos },
  { 4, Roboto_Medium12_lparen },
  { 4, Roboto_Medium12_rparen },
  { 5, Roboto_Medium12_asterisk },
  { 6, Roboto_Medium12_plus },
  { 2, Roboto_Medium12_comma },
  { 3, Roboto_Medium12_minus },
  { 3, Roboto_Medium12_dot },
  { 4, Roboto_Medium12_slash },
  { 6, Roboto_Medium12_0 },
  { 6, Roboto_Medium12_1 },
  { 6, Roboto_Medium12_2 },
  { 6, Roboto_Medium12_3 },
  { 6, Roboto_Medium12_4 },
  { 6, Roboto_Medium12_5 },
  { 6, Roboto_Medium12_6 },
  { 6, Roboto_Medium12_7 },
  { 6, Roboto_Medium12_8 },
  { 6, Roboto_Medium12_9 },
  { 3, Roboto_Medium12_colon },
  { 2, Roboto_Medium12_semi },
  { 5, Roboto_Medium12_lt },
  { 6, Roboto_Medium12_eq },
  { 5, Roboto_Medium12_gt },
  { 5, Roboto_Medium12_qmark },
  { 9, Roboto_Medium12_at },
  { 7, Roboto_Medium12_A },
  { 6, Roboto_Medium12_B },
  { 7, Roboto_Medium12_C },
  { 7, Roboto_Medium12_D },
  { 6, Roboto_Medium12_E },
  { 6, Roboto_Medium12_F },
  { 7, Roboto_Medium12_G },
  { 7, Roboto_Medium12_H },
  { 3, Roboto_Medium12_I },
  { 6, Roboto_Medium12_J },
  { 7, Roboto_Medium12_K },
  { 6, Roboto_Medium12_L },
  { 9, Roboto_Medium12_M },
  { 7, Roboto_Medium12_N },
  { 7, Roboto_Medium12_O },
  { 7, Roboto_Medium12_P },
  { 7, Roboto_Medium12_Q },
  { 7, Roboto_Medium12_R },
  { 6, Roboto_Medium12_S },
  { 6, Roboto_Medium12_T },
  { 7, Roboto_Medium12_U },
  { 7, Roboto_Medium12_V },
  { 9, Roboto_Medium12_W },
  { 7, Roboto_Medium12_X },
  { 7, Roboto_Medium12_Y },
  { 6, Roboto_Medium12_Z },
  { 3, Roboto_Medium12_lbrack },
  { 5, Roboto_Medium12_bslash },
  { 3, Roboto_Medium12_rbrack },
  { 5, Roboto_Medium12_caret },
  { 5, Roboto_Medium12_underscore },
  { 3, Roboto_Medium12_backtick },
  { 5, Roboto_Medium12_a },
  { 6, Roboto_Medium12_b },
  { 5, Roboto_Medium12_c },
  { 6, Roboto_Medium12_d },
  { 6, Roboto_Medium12_e },
  { 4, Roboto_Medium12_f },
  { 6, Roboto_Medium12_g },
  { 6, Roboto_Medium12_h },
  { 3, Roboto_Medium12_i },
  { 4, Roboto_Medium12_j },
  { 6, Roboto_Medium12_k },
  { 3, Roboto_Medium12_l },
  { 9, Roboto_Medium12_m },
  { 6, Roboto_Medium12_n },
  { 6, Roboto_Medium12_o },
  { 6, Roboto_Medium12_p },
  { 6, Roboto_Medium12_q },
  { 4, Roboto_Medium12_r },
  { 5, Roboto_Medium12_s },
  { 4, Roboto_Medium12_t },
  { 6, Roboto_Medium12_u },
  { 5, Roboto_Medium12_v },
  { 8, Roboto_Medium12_w },
  { 5, Roboto_Medium12_x },
  { 5, Roboto_Medium12_y },
  { 5, Roboto_Medium12_z },
  { 4, Roboto_Medium12_lbrace },
  { 2, Roboto_Medium12_pipe },
  { 4, Roboto_Medium12_rbrace },
  { 7, Roboto_Medium12_tilde },
};

vFONT Roboto_Medium12 = {
  Roboto_Medium12_Table,
  2,
  12
};
