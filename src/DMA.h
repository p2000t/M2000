/****************************************************************************/
/**                                                                        **/
/**                                  DMA.h                                 **/
/**                                                                        **/
/** DMA memory allocation and I/O routines                                 **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996                                     **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/* _dma_free_mem() **********************************************************/
/* Frees DOS memory allocated by _dma_allocate_mem()                        */
/****************************************************************************/
int _dma_free_mem (unsigned sel);

/* _dma_allocate_mem() ******************************************************/
/* Allocates the specified amount of conventional memory, ensuring that     */
/* the returned block doesn't cross a page boundary. Sel will be set to     */
/* the protected mode segment that should be used to free the block, and    */
/* phys to the linear address of the block. On error, returns non-zero      */
/* and sets sel and phys to 0                                               */
/****************************************************************************/
int _dma_allocate_mem(unsigned bytes,unsigned *sel,
                      unsigned *offs,unsigned *phys);

/* dma_start() **************************************************************/
/* Starts the DMA controller for the specified channel, transferring        */
/* size bytes from addr (the block must not cross a page boundary).         */
/* If auto_init is set, it will use the endless repeat DMA mode             */
/****************************************************************************/
void _dma_start(unsigned channel,unsigned addr,unsigned size,int auto_init);

/* dma_stop() ***************************************************************/
/* Disables the specified DMA channel                                       */
/****************************************************************************/
void _dma_stop(unsigned channel);

/* dma_todo() ***************************************************************/
/* Returns the current position in a dma transfer                           */
/****************************************************************************/
unsigned _dma_todo(unsigned channel);
