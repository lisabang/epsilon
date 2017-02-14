#include "sequence_store.h"
extern "C" {
#include <assert.h>
#include <stddef.h>
}
#include <ion.h>

namespace Sequence {

constexpr KDColor SequenceStore::k_defaultColors[k_maxNumberOfSequences];
constexpr const char * SequenceStore::k_sequenceNames[k_maxNumberOfSequences];

uint32_t SequenceStore::storeChecksum() {
  size_t dataLengthInBytes = m_numberOfFunctions*sizeof(Sequence);
  assert((dataLengthInBytes & 0x3) == 0); // Assert that dataLengthInBytes is a multiple of 4
  return Ion::crc32((uint32_t *)m_sequences, dataLengthInBytes>>2);
}

Sequence * SequenceStore::functionAtIndex(int i) {
  assert(i>=0 && i<m_numberOfFunctions);
  return &m_sequences[i];
}

Sequence * SequenceStore::activeFunctionAtIndex(int i) {
  return (Sequence *)Shared::FunctionStore::activeFunctionAtIndex(i);
}

Sequence * SequenceStore::definedFunctionAtIndex(int i) {
  return (Sequence *)Shared::FunctionStore::definedFunctionAtIndex(i);
}

Sequence * SequenceStore::addEmptyFunction() {
  assert(m_numberOfFunctions < k_maxNumberOfSequences);
  const char * name = firstAvailableName();
  KDColor color = firstAvailableColor();
  Sequence addedSequence = Sequence(name, color);
  m_sequences[m_numberOfFunctions] = addedSequence;
  Sequence * result = &m_sequences[m_numberOfFunctions];
  m_numberOfFunctions++;
  return result;
}

void SequenceStore::removeFunction(Shared::Function * f) {
  int i = 0;
  while (&m_sequences[i] != f && i < m_numberOfFunctions) {
    i++;
  }
  assert(i>=0 && i<m_numberOfFunctions);
  m_numberOfFunctions--;
  for (int j = i; j<m_numberOfFunctions; j++) {
    m_sequences[j] = m_sequences[j+1];
  }
}

const char *  SequenceStore::firstAvailableName() {
  for (int k = 0; k < k_maxNumberOfSequences; k++) {
    int j = 0;
    while  (j < m_numberOfFunctions) {
      if (m_sequences[j].name() == k_sequenceNames[k]) {
        break;
      }
      j++;
    }
    if (j == m_numberOfFunctions) {
      return k_sequenceNames[k];
    }
  }
  return k_sequenceNames[0];
}

const KDColor SequenceStore::firstAvailableColor() {
  for (int k = 0; k < k_maxNumberOfSequences; k++) {
    int j = 0;
    while  (j < m_numberOfFunctions) {
      if (m_sequences[j].color() == k_defaultColors[k]) {
        break;
      }
      j++;
    }
    if (j == m_numberOfFunctions) {
      return k_defaultColors[k];
    }
  }
  return k_defaultColors[0];
}

}