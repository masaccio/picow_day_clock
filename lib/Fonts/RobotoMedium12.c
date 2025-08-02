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

static const uint8_t Roboto_Medium12_32[] = {
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_33[] = {
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_34[] = {
  0x00, //     
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
};

static const uint8_t Roboto_Medium12_35[] = {
  0x00, //         
  0x14, //    # #  
  0x14, //    # #  
  0x14, //    # #  
  0x7E, //  ###### 
  0x28, //   # #   
  0xFE, // ####### 
  0x28, //   # #   
  0x28, //   # #   
  0x48, //  #  #   
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_36[] = {
  0x10, //    #   
  0x38, //   ###  
  0x6C, //  ## ## 
  0x44, //  #   # 
  0x60, //  ##    
  0x38, //   ###  
  0x0C, //     ## 
  0x44, //  #   # 
  0x6C, //  ## ## 
  0x38, //   ###  
  0x10, //    #   
  0x00, //        
};

static const uint8_t Roboto_Medium12_37[] = {
  0x00, //         
  0x00, //          
  0x70, //  ###    
  0x00, //  ###     
  0x52, //  # #  # 
  0x00, //  # #  #  
  0x54, //  # # #  
  0x00, //  # # #   
  0x74, //  ### #  
  0x00, //  ### #   
  0x08, //     #   
  0x00, //     #    
  0x17, //    # ###
  0x00, //    # ### 
  0x15, //    # # #
  0x00, //    # # # 
  0x25, //   #  # #
  0x00, //   #  # # 
  0x07, //      ###
  0x00, //      ### 
  0x00, //         
  0x00, //          
  0x00, //         
  0x00, //          
};

static const uint8_t Roboto_Medium12_38[] = {
  0x00, //         
  0x38, //   ###   
  0x6C, //  ## ##  
  0x68, //  ## #   
  0x38, //   ###   
  0x30, //   ##    
  0x5A, //  # ## # 
  0xCE, // ##  ### 
  0x44, //  #   #  
  0x7A, //  #### # 
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_39[] = {
  0x00, //   
  0xC0, // ##
  0xC0, // ##
  0x80, // # 
  0x00, //   
  0x00, //   
  0x00, //   
  0x00, //   
  0x00, //   
  0x00, //   
  0x00, //   
  0x00, //   
};

static const uint8_t Roboto_Medium12_40[] = {
  0x10, //    #
  0x20, //   # 
  0x20, //   # 
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x20, //   # 
  0x20, //   # 
};

static const uint8_t Roboto_Medium12_41[] = {
  0x80, // #   
  0x40, //  #  
  0x60, //  ## 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x60, //  ## 
  0x40, //  #  
};

static const uint8_t Roboto_Medium12_42[] = {
  0x00, //       
  0x20, //   #   
  0xA0, // # #   
  0x78, //  #### 
  0x70, //  ###  
  0x50, //  # #  
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_43[] = {
  0x00, //        
  0x00, //        
  0x00, //        
  0x10, //    #   
  0x10, //    #   
  0xFC, // ###### 
  0x10, //    #   
  0x10, //    #   
  0x10, //    #   
  0x00, //        
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_44[] = {
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x80, // #  
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_45[] = {
  0x00, //     
  0x00, //     
  0x00, //     
  0xF0, // ####
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
};

static const uint8_t Roboto_Medium12_46[] = {
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_47[] = {
  0x00, //      
  0x10, //    # 
  0x10, //    # 
  0x30, //   ## 
  0x20, //   #  
  0x20, //   #  
  0x60, //  ##  
  0x40, //  #   
  0x40, //  #   
  0xC0, // ##   
  0x80, // #    
  0x00, //      
};

static const uint8_t Roboto_Medium12_48[] = {
  0x00, //        
  0x38, //   ###  
  0x6C, //  ## ## 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x6C, //  ## ## 
  0x38, //   ###  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_49[] = {
  0x00, //        
  0x18, //    ##  
  0x78, //  ####  
  0x18, //    ##  
  0x18, //    ##  
  0x18, //    ##  
  0x18, //    ##  
  0x18, //    ##  
  0x18, //    ##  
  0x18, //    ##  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_50[] = {
  0x00, //        
  0x38, //   ###  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0x0C, //     ## 
  0x08, //     #  
  0x10, //    #   
  0x30, //   ##   
  0x60, //  ##    
  0x7C, //  ##### 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_51[] = {
  0x00, //        
  0x78, //  ####  
  0x4C, //  #  ## 
  0x04, //      # 
  0x0C, //     ## 
  0x38, //   ###  
  0x0C, //     ## 
  0x04, //      # 
  0x4C, //  #  ## 
  0x78, //  ####  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_52[] = {
  0x00, //        
  0x08, //     #  
  0x18, //    ##  
  0x18, //    ##  
  0x28, //   # #  
  0x68, //  ## #  
  0x48, //  #  #  
  0xFE, // #######
  0x08, //     #  
  0x08, //     #  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_53[] = {
  0x00, //        
  0x7C, //  ##### 
  0x60, //  ##    
  0x40, //  #     
  0x78, //  ####  
  0x0C, //     ## 
  0x04, //      # 
  0x04, //      # 
  0x4C, //  #  ## 
  0x38, //   ###  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_54[] = {
  0x00, //        
  0x18, //    ##  
  0x20, //   #    
  0x40, //  #     
  0x78, //  ####  
  0x6C, //  ## ## 
  0x44, //  #   # 
  0x44, //  #   # 
  0x6C, //  ## ## 
  0x38, //   ###  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_55[] = {
  0x00, //        
  0xFC, // ###### 
  0x04, //      # 
  0x0C, //     ## 
  0x08, //     #  
  0x18, //    ##  
  0x10, //    #   
  0x30, //   ##   
  0x30, //   ##   
  0x20, //   #    
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_56[] = {
  0x00, //        
  0x38, //   ###  
  0x6C, //  ## ## 
  0x44, //  #   # 
  0x6C, //  ## ## 
  0x38, //   ###  
  0x44, //  #   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x38, //   ###  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_57[] = {
  0x00, //        
  0x38, //   ###  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x34, //   ## # 
  0x0C, //     ## 
  0x18, //    ##  
  0x30, //   ##   
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_58[] = {
  0x00, //    
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_59[] = {
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
  0x00, //    
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0xC0, // ## 
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_60[] = {
  0x00, //       
  0x00, //       
  0x00, //       
  0x08, //     # 
  0x38, //   ### 
  0xC0, // ##    
  0x38, //   ### 
  0x08, //     # 
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_61[] = {
  0x00, //        
  0x00, //        
  0x7C, //  ##### 
  0x00, //        
  0x00, //        
  0x7C, //  ##### 
  0x00, //        
  0x00, //        
  0x00, //        
  0x00, //        
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_62[] = {
  0x00, //       
  0x00, //       
  0x00, //       
  0x40, //  #    
  0x78, //  #### 
  0x0C, //     ##
  0x78, //  #### 
  0x40, //  #    
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_63[] = {
  0x00, //       
  0x78, //  #### 
  0xC8, // ##  # 
  0x08, //     # 
  0x18, //    ## 
  0x30, //   ##  
  0x20, //   #   
  0x00, //       
  0x00, //       
  0x30, //   ##  
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_64[] = {
  0x1F, //    #####
  0x00, //    #####   
  0x20, //   #     
  0x80, //   #     #  
  0x60, //  ##     
  0x40, //  ##      # 
  0x4E, //  #  ### 
  0x40, //  #  ###  # 
  0x4A, //  #  # # 
  0x40, //  #  # #  # 
  0x52, //  # #  # 
  0x40, //  # #  #  # 
  0x52, //  # #  # 
  0x40, //  # #  #  # 
  0x4F, //  #  ####
  0x80, //  #  #####  
  0x40, //  #      
  0x00, //  #         
  0x20, //   #     
  0x00, //   #        
  0x1E, //    #### 
  0x00, //    ####    
  0x00, //         
  0x00, //            
};

static const uint8_t Roboto_Medium12_65[] = {
  0x00, //         
  0x18, //    ##   
  0x18, //    ##   
  0x3C, //   ####  
  0x24, //   #  #  
  0x24, //   #  #  
  0x66, //  ##  ## 
  0x7E, //  ###### 
  0x42, //  #    # 
  0xC3, // ##    ##
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_66[] = {
  0x00, //         
  0x7C, //  #####  
  0x46, //  #   ## 
  0x46, //  #   ## 
  0x46, //  #   ## 
  0x7C, //  #####  
  0x46, //  #   ## 
  0x46, //  #   ## 
  0x46, //  #   ## 
  0x7C, //  #####  
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_67[] = {
  0x00, //         
  0x3C, //   ####  
  0x66, //  ##  ## 
  0x42, //  #    # 
  0x40, //  #      
  0x40, //  #      
  0x40, //  #      
  0x42, //  #    # 
  0x66, //  ##  ## 
  0x3C, //   ####  
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_68[] = {
  0x00, //         
  0x78, //  ####   
  0x44, //  #   #  
  0x46, //  #   ## 
  0x42, //  #    # 
  0x42, //  #    # 
  0x42, //  #    # 
  0x46, //  #   ## 
  0x44, //  #   #  
  0x78, //  ####   
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_69[] = {
  0x00, //        
  0x7C, //  ##### 
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x7C, //  ##### 
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x7C, //  ##### 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_70[] = {
  0x00, //        
  0x7C, //  ##### 
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x7C, //  ##### 
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_71[] = {
  0x00, //         
  0x3C, //   ####  
  0x66, //  ##  ## 
  0x42, //  #    # 
  0x40, //  #      
  0x40, //  #      
  0x4E, //  #  ### 
  0x42, //  #    # 
  0x62, //  ##   # 
  0x3E, //   ##### 
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_72[] = {
  0x00, //         
  0x00, //          
  0x43, //  #    ##
  0x00, //  #    ## 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x7F, //  #######
  0x00, //  ####### 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x00, //         
  0x00, //          
  0x00, //         
  0x00, //          
};

static const uint8_t Roboto_Medium12_73[] = {
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_74[] = {
  0x00, //        
  0x0C, //     ## 
  0x0C, //     ## 
  0x0C, //     ## 
  0x0C, //     ## 
  0x0C, //     ## 
  0x0C, //     ## 
  0xCC, // ##  ## 
  0x4C, //  #  ## 
  0x78, //  ####  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_75[] = {
  0x00, //         
  0x46, //  #   ## 
  0x4C, //  #  ##  
  0x48, //  #  #   
  0x58, //  # ##   
  0x78, //  ####   
  0x68, //  ## #   
  0x4C, //  #  ##  
  0x46, //  #   ## 
  0x42, //  #    # 
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_76[] = {
  0x00, //        
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x40, //  #     
  0x7C, //  ##### 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_77[] = {
  0x00, //         
  0x00, //            
  0x60, //  ##     
  0xC0, //  ##     ## 
  0x61, //  ##    #
  0xC0, //  ##    ### 
  0x71, //  ###   #
  0xC0, //  ###   ### 
  0x53, //  # #  ##
  0xC0, //  # #  #### 
  0x52, //  # #  # 
  0xC0, //  # #  # ## 
  0x5A, //  # ## # 
  0xC0, //  # ## # ## 
  0x4E, //  #  ### 
  0xC0, //  #  ### ## 
  0x4C, //  #  ##  
  0xC0, //  #  ##  ## 
  0x44, //  #   #  
  0xC0, //  #   #  ## 
  0x00, //         
  0x00, //            
  0x00, //         
  0x00, //            
};

static const uint8_t Roboto_Medium12_78[] = {
  0x00, //         
  0x00, //          
  0x63, //  ##   ##
  0x00, //  ##   ## 
  0x63, //  ##   ##
  0x00, //  ##   ## 
  0x73, //  ###  ##
  0x00, //  ###  ## 
  0x53, //  # #  ##
  0x00, //  # #  ## 
  0x4B, //  #  # ##
  0x00, //  #  # ## 
  0x4F, //  #  ####
  0x00, //  #  #### 
  0x47, //  #   ###
  0x00, //  #   ### 
  0x47, //  #   ###
  0x00, //  #   ### 
  0x43, //  #    ##
  0x00, //  #    ## 
  0x00, //         
  0x00, //          
  0x00, //         
  0x00, //          
};

static const uint8_t Roboto_Medium12_79[] = {
  0x00, //         
  0x3C, //   ####  
  0x66, //  ##  ## 
  0x42, //  #    # 
  0x43, //  #    ##
  0x43, //  #    ##
  0x43, //  #    ##
  0x42, //  #    # 
  0x66, //  ##  ## 
  0x3C, //   ####  
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_80[] = {
  0x00, //         
  0x7C, //  #####  
  0x46, //  #   ## 
  0x42, //  #    # 
  0x42, //  #    # 
  0x46, //  #   ## 
  0x7C, //  #####  
  0x40, //  #      
  0x40, //  #      
  0x40, //  #      
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_81[] = {
  0x00, //         
  0x3C, //   ####  
  0x66, //  ##  ## 
  0x42, //  #    # 
  0x43, //  #    ##
  0x43, //  #    ##
  0x43, //  #    ##
  0x42, //  #    # 
  0x66, //  ##  ## 
  0x3C, //   ####  
  0x02, //       # 
  0x00, //         
};

static const uint8_t Roboto_Medium12_82[] = {
  0x00, //         
  0x7C, //  #####  
  0x46, //  #   ## 
  0x46, //  #   ## 
  0x46, //  #   ## 
  0x46, //  #   ## 
  0x7C, //  #####  
  0x4C, //  #  ##  
  0x44, //  #   #  
  0x46, //  #   ## 
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_83[] = {
  0x00, //        
  0x3C, //   #### 
  0x64, //  ##  # 
  0x46, //  #   ##
  0x60, //  ##    
  0x3C, //   #### 
  0x06, //      ##
  0xC6, // ##   ##
  0x66, //  ##  ##
  0x3C, //   #### 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_84[] = {
  0x00, //         
  0xFE, // ####### 
  0x10, //    #    
  0x10, //    #    
  0x10, //    #    
  0x10, //    #    
  0x10, //    #    
  0x10, //    #    
  0x10, //    #    
  0x10, //    #    
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_85[] = {
  0x00, //         
  0x42, //  #    # 
  0x42, //  #    # 
  0x42, //  #    # 
  0x42, //  #    # 
  0x42, //  #    # 
  0x42, //  #    # 
  0x42, //  #    # 
  0x66, //  ##  ## 
  0x3C, //   ####  
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_86[] = {
  0x00, //         
  0xC2, // ##    # 
  0x42, //  #    # 
  0x46, //  #   ## 
  0x64, //  ##  #  
  0x24, //   #  #  
  0x2C, //   # ##  
  0x38, //   ###   
  0x18, //    ##   
  0x18, //    ##   
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_87[] = {
  0x00, //         
  0x00, //            
  0xC4, // ##   #  
  0x40, // ##   #   # 
  0x4C, //  #  ##  
  0x40, //  #  ##   # 
  0x4E, //  #  ### 
  0xC0, //  #  ### ## 
  0x4A, //  #  # # 
  0xC0, //  #  # # ## 
  0x6A, //  ## # # 
  0x80, //  ## # # #  
  0x7A, //  #### # 
  0x80, //  #### # #  
  0x33, //   ##  ##
  0x80, //   ##  ###  
  0x31, //   ##   #
  0x80, //   ##   ##  
  0x31, //   ##   #
  0x00, //   ##   #   
  0x00, //         
  0x00, //            
  0x00, //         
  0x00, //            
};

static const uint8_t Roboto_Medium12_88[] = {
  0x00, //         
  0x46, //  #   ## 
  0x64, //  ##  #  
  0x2C, //   # ##  
  0x38, //   ###   
  0x18, //    ##   
  0x38, //   ###   
  0x2C, //   # ##  
  0x64, //  ##  #  
  0x46, //  #   ## 
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_89[] = {
  0x00, //         
  0xC6, // ##   ## 
  0x46, //  #   ## 
  0x6C, //  ## ##  
  0x2C, //   # ##  
  0x38, //   ###   
  0x18, //    ##   
  0x10, //    #    
  0x10, //    #    
  0x10, //    #    
  0x00, //         
  0x00, //         
};

static const uint8_t Roboto_Medium12_90[] = {
  0x00, //        
  0xFE, // #######
  0x04, //      # 
  0x0C, //     ## 
  0x18, //    ##  
  0x10, //    #   
  0x30, //   ##   
  0x60, //  ##    
  0x40, //  #     
  0xFE, // #######
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_91[] = {
  0x60, //  ## 
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x40, //  #  
  0x60, //  ## 
};

static const uint8_t Roboto_Medium12_92[] = {
  0x00, //       
  0xC0, // ##    
  0x40, //  #    
  0x40, //  #    
  0x60, //  ##   
  0x20, //   #   
  0x20, //   #   
  0x30, //   ##  
  0x10, //    #  
  0x18, //    ## 
  0x18, //    ## 
  0x00, //       
};

static const uint8_t Roboto_Medium12_93[] = {
  0xC0, // ## 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0xC0, // ## 
};

static const uint8_t Roboto_Medium12_94[] = {
  0x00, //      
  0x20, //   #  
  0x70, //  ### 
  0x50, //  # # 
  0x58, //  # ##
  0x00, //      
  0x00, //      
  0x00, //      
  0x00, //      
  0x00, //      
  0x00, //      
  0x00, //      
};

static const uint8_t Roboto_Medium12_95[] = {
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
  0xF8, // ##### 
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_96[] = {
  0x00, //     
  0x40, //  #  
  0x60, //  ## 
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
  0x00, //     
};

static const uint8_t Roboto_Medium12_97[] = {
  0x00, //       
  0x00, //       
  0x78, //  #### 
  0x4C, //  #  ##
  0x0C, //     ##
  0x7C, //  #####
  0x4C, //  #  ##
  0x4C, //  #  ##
  0x7C, //  #####
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_98[] = {
  0x00, //        
  0x40, //  #     
  0x40, //  #     
  0x78, //  ####  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x6C, //  ## ## 
  0x78, //  ####  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_99[] = {
  0x00, //       
  0x00, //       
  0x38, //   ### 
  0x4C, //  #  ##
  0x40, //  #    
  0xC0, // ##    
  0x40, //  #    
  0x4C, //  #  ##
  0x38, //   ### 
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_100[] = {
  0x00, //        
  0x04, //      # 
  0x04, //      # 
  0x34, //   ## # 
  0x4C, //  #  ## 
  0x44, //  #   # 
  0xC4, // ##   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x34, //   ## # 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_101[] = {
  0x00, //        
  0x00, //        
  0x38, //   ###  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0xFC, // ###### 
  0x40, //  #     
  0x64, //  ##  # 
  0x38, //   ###  
  0x00, //        
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_102[] = {
  0x00, //      
  0x30, //   ## 
  0x60, //  ##  
  0xF0, // #### 
  0x60, //  ##  
  0x60, //  ##  
  0x60, //  ##  
  0x60, //  ##  
  0x60, //  ##  
  0x60, //  ##  
  0x00, //      
  0x00, //      
};

static const uint8_t Roboto_Medium12_103[] = {
  0x00, //        
  0x34, //   ## # 
  0x4C, //  #  ## 
  0x44, //  #   # 
  0xC4, // ##   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x34, //   ## # 
  0x4C, //  #  ## 
  0x78, //  ####  
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_104[] = {
  0x00, //        
  0x40, //  #     
  0x40, //  #     
  0x78, //  ####  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_105[] = {
  0x00, //    
  0x40, //  # 
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_106[] = {
  0x20, //   # 
  0x00, //     
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0x20, //   # 
  0xE0, // ### 
  0x00, //     
};

static const uint8_t Roboto_Medium12_107[] = {
  0x00, //        
  0x40, //  #     
  0x40, //  #     
  0x4C, //  #  ## 
  0x58, //  # ##  
  0x70, //  ###   
  0x70, //  ###   
  0x58, //  # ##  
  0x48, //  #  #  
  0x4C, //  #  ## 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_108[] = {
  0x00, //    
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x00, //    
  0x00, //    
};

static const uint8_t Roboto_Medium12_109[] = {
  0x00, //         
  0x00, //           
  0x00, //         
  0x00, //           
  0x7B, //  #### ##
  0x80, //  #### ### 
  0x4C, //  #  ##  
  0xC0, //  #  ##  ##
  0x44, //  #   #  
  0xC0, //  #   #  ##
  0x44, //  #   #  
  0xC0, //  #   #  ##
  0x44, //  #   #  
  0xC0, //  #   #  ##
  0x44, //  #   #  
  0xC0, //  #   #  ##
  0x44, //  #   #  
  0xC0, //  #   #  ##
  0x00, //         
  0x00, //           
  0x00, //         
  0x00, //           
  0x00, //         
  0x00, //           
};

static const uint8_t Roboto_Medium12_110[] = {
  0x00, //        
  0x00, //        
  0x78, //  ####  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x00, //        
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_111[] = {
  0x00, //        
  0x00, //        
  0x38, //   ###  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0xC4, // ##   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x38, //   ###  
  0x00, //        
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_112[] = {
  0x00, //        
  0x78, //  ####  
  0x4C, //  #  ## 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x78, //  ####  
  0x40, //  #     
  0x40, //  #     
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_113[] = {
  0x00, //        
  0x34, //   ## # 
  0x4C, //  #  ## 
  0x44, //  #   # 
  0xC4, // ##   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x34, //   ## # 
  0x04, //      # 
  0x04, //      # 
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_114[] = {
  0x00, //      
  0x00, //      
  0x70, //  ### 
  0x60, //  ##  
  0x40, //  #   
  0x40, //  #   
  0x40, //  #   
  0x40, //  #   
  0x40, //  #   
  0x00, //      
  0x00, //      
  0x00, //      
};

static const uint8_t Roboto_Medium12_115[] = {
  0x00, //       
  0x00, //       
  0x78, //  #### 
  0x4C, //  #  ##
  0x40, //  #    
  0x78, //  #### 
  0x0C, //     ##
  0x4C, //  #  ##
  0x78, //  #### 
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_116[] = {
  0x00, //     
  0x00, //     
  0x60, //  ## 
  0xF0, // ####
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0x70, //  ###
  0x00, //     
  0x00, //     
};

static const uint8_t Roboto_Medium12_117[] = {
  0x00, //        
  0x00, //        
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x44, //  #   # 
  0x4C, //  #  ## 
  0x74, //  ### # 
  0x00, //        
  0x00, //        
  0x00, //        
};

static const uint8_t Roboto_Medium12_118[] = {
  0x00, //       
  0x00, //       
  0xCC, // ##  ##
  0x48, //  #  # 
  0x48, //  #  # 
  0x78, //  #### 
  0x30, //   ##  
  0x30, //   ##  
  0x30, //   ##  
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_119[] = {
  0x00, //         
  0x00, //          
  0x00, //         
  0x00, //          
  0xC9, // ##  #  #
  0x80, // ##  #  ##
  0x49, //  #  #  #
  0x00, //  #  #  # 
  0x55, //  # # # #
  0x00, //  # # # # 
  0x55, //  # # # #
  0x00, //  # # # # 
  0x77, //  ### ###
  0x00, //  ### ### 
  0x36, //   ## ## 
  0x00, //   ## ##  
  0x22, //   #   # 
  0x00, //   #   #  
  0x00, //         
  0x00, //          
  0x00, //         
  0x00, //          
  0x00, //         
  0x00, //          
};

static const uint8_t Roboto_Medium12_120[] = {
  0x00, //       
  0x00, //       
  0x4C, //  #  ##
  0x78, //  #### 
  0x30, //   ##  
  0x30, //   ##  
  0x30, //   ##  
  0x68, //  ## # 
  0xCC, // ##  ##
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_121[] = {
  0x00, //       
  0xCC, // ##  ##
  0x48, //  #  # 
  0x48, //  #  # 
  0x78, //  #### 
  0x30, //   ##  
  0x30, //   ##  
  0x30, //   ##  
  0x20, //   #   
  0x60, //  ##   
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_122[] = {
  0x00, //       
  0x00, //       
  0x7C, //  #####
  0x08, //     # 
  0x10, //    #  
  0x30, //   ##  
  0x20, //   #   
  0x40, //  #    
  0xFC, // ######
  0x00, //       
  0x00, //       
  0x00, //       
};

static const uint8_t Roboto_Medium12_123[] = {
  0x30, //   ##
  0x20, //   # 
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0xC0, // ##  
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0x20, //   # 
  0x30, //   ##
  0x00, //     
};

static const uint8_t Roboto_Medium12_124[] = {
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x40, //  # 
  0x00, //    
};

static const uint8_t Roboto_Medium12_125[] = {
  0xC0, // ##  
  0x40, //  #  
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0x30, //   ##
  0x60, //  ## 
  0x60, //  ## 
  0x60, //  ## 
  0x40, //  #  
  0xC0, // ##  
  0x00, //     
};

static const uint8_t Roboto_Medium12_126[] = {
  0x00, //         
  0x00, //         
  0x00, //         
  0x72, //  ###  # 
  0x4E, //  #  ### 
  0x00, //         
  0x00, //         
  0x00, //         
  0x00, //         
  0x00, //         
  0x00, //         
  0x00, //         
};

static const vFONTENTRY Roboto_Medium12_Table[] = {
  { 3, Roboto_Medium12_32 },
  { 3, Roboto_Medium12_33 },
  { 4, Roboto_Medium12_34 },
  { 8, Roboto_Medium12_35 },
  { 7, Roboto_Medium12_36 },
  { 9, Roboto_Medium12_37 },
  { 8, Roboto_Medium12_38 },
  { 2, Roboto_Medium12_39 },
  { 4, Roboto_Medium12_40 },
  { 4, Roboto_Medium12_41 },
  { 6, Roboto_Medium12_42 },
  { 7, Roboto_Medium12_43 },
  { 3, Roboto_Medium12_44 },
  { 4, Roboto_Medium12_45 },
  { 3, Roboto_Medium12_46 },
  { 5, Roboto_Medium12_47 },
  { 7, Roboto_Medium12_48 },
  { 7, Roboto_Medium12_49 },
  { 7, Roboto_Medium12_50 },
  { 7, Roboto_Medium12_51 },
  { 7, Roboto_Medium12_52 },
  { 7, Roboto_Medium12_53 },
  { 7, Roboto_Medium12_54 },
  { 7, Roboto_Medium12_55 },
  { 7, Roboto_Medium12_56 },
  { 7, Roboto_Medium12_57 },
  { 3, Roboto_Medium12_58 },
  { 3, Roboto_Medium12_59 },
  { 6, Roboto_Medium12_60 },
  { 7, Roboto_Medium12_61 },
  { 6, Roboto_Medium12_62 },
  { 6, Roboto_Medium12_63 },
  { 11, Roboto_Medium12_64 },
  { 8, Roboto_Medium12_65 },
  { 8, Roboto_Medium12_66 },
  { 8, Roboto_Medium12_67 },
  { 8, Roboto_Medium12_68 },
  { 7, Roboto_Medium12_69 },
  { 7, Roboto_Medium12_70 },
  { 8, Roboto_Medium12_71 },
  { 9, Roboto_Medium12_72 },
  { 3, Roboto_Medium12_73 },
  { 7, Roboto_Medium12_74 },
  { 8, Roboto_Medium12_75 },
  { 7, Roboto_Medium12_76 },
  { 11, Roboto_Medium12_77 },
  { 9, Roboto_Medium12_78 },
  { 8, Roboto_Medium12_79 },
  { 8, Roboto_Medium12_80 },
  { 8, Roboto_Medium12_81 },
  { 8, Roboto_Medium12_82 },
  { 7, Roboto_Medium12_83 },
  { 8, Roboto_Medium12_84 },
  { 8, Roboto_Medium12_85 },
  { 8, Roboto_Medium12_86 },
  { 11, Roboto_Medium12_87 },
  { 8, Roboto_Medium12_88 },
  { 8, Roboto_Medium12_89 },
  { 7, Roboto_Medium12_90 },
  { 4, Roboto_Medium12_91 },
  { 6, Roboto_Medium12_92 },
  { 3, Roboto_Medium12_93 },
  { 5, Roboto_Medium12_94 },
  { 6, Roboto_Medium12_95 },
  { 4, Roboto_Medium12_96 },
  { 6, Roboto_Medium12_97 },
  { 7, Roboto_Medium12_98 },
  { 6, Roboto_Medium12_99 },
  { 7, Roboto_Medium12_100 },
  { 7, Roboto_Medium12_101 },
  { 5, Roboto_Medium12_102 },
  { 7, Roboto_Medium12_103 },
  { 7, Roboto_Medium12_104 },
  { 3, Roboto_Medium12_105 },
  { 4, Roboto_Medium12_106 },
  { 7, Roboto_Medium12_107 },
  { 3, Roboto_Medium12_108 },
  { 10, Roboto_Medium12_109 },
  { 7, Roboto_Medium12_110 },
  { 7, Roboto_Medium12_111 },
  { 7, Roboto_Medium12_112 },
  { 7, Roboto_Medium12_113 },
  { 5, Roboto_Medium12_114 },
  { 6, Roboto_Medium12_115 },
  { 4, Roboto_Medium12_116 },
  { 7, Roboto_Medium12_117 },
  { 6, Roboto_Medium12_118 },
  { 9, Roboto_Medium12_119 },
  { 6, Roboto_Medium12_120 },
  { 6, Roboto_Medium12_121 },
  { 6, Roboto_Medium12_122 },
  { 4, Roboto_Medium12_123 },
  { 3, Roboto_Medium12_124 },
  { 4, Roboto_Medium12_125 },
  { 8, Roboto_Medium12_126 },
};

vFONT Roboto_Medium12 = {
  Roboto_Medium12_Table,
  12
};
