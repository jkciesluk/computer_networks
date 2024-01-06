/*
Jakub Ciesluk
323892
*/
#include "window.h"

void init_window(window_t *window, uint32_t filesize)
{
  window->start = 0;
  window->filesize = filesize;
  window->end = filesize % SEGMENT_SIZE == 0 ? filesize / SEGMENT_SIZE : filesize / SEGMENT_SIZE + 1;
  window->size = window->end > WINDOW_SIZE ? WINDOW_SIZE : window->end;
  for (int i = 0; i < MAX_SEGMENTS; i++)
    window->acks[i] = 0;
}

void set_ack(window_t *window, uint32_t n)
{
  window->acks[n] = 1;
}

void shift_and_write(window_t *window, FILE *file)
{
  if (!all_set(window))
  {
    return;
  }
  if (window->start + window->size >= window->end)
  {
    fwrite(window->data, 1, window->filesize - window->start * SEGMENT_SIZE, file);
  }
  else
  {
    fwrite(window->data, 1, window->size * SEGMENT_SIZE, file);
  }
  window->start += window->size;
  for (uint32_t i = 0; i < WINDOW_SIZE; i++)
  {
    window->acks[i] = 0;
  }
  window->size = window->end - window->start > WINDOW_SIZE ? WINDOW_SIZE : window->end - window->start;
}

uint32_t is_set(window_t *window, uint32_t n)
{
  return window->acks[n];
}

uint32_t all_set(window_t *window)
{
  uint32_t set;
  for (uint32_t i = 0; i < window->size; i++)
  {
    set = is_set(window, i);
    if (!set)
    {
      return 0;
    }
  }
  return 1;
}