# Polymorphic-Memory-Resources
A learning experience with memory management...

Example Output from main():

  syncpool allocate 8 Bytes
keeppool: allocate 10000 Bytes
  syncpool allocate 24 Bytes
  syncpool allocate 92 Bytes
  syncpool allocate 48 Bytes
  syncpool deallocate 24 Bytes
  syncpool allocate 16412 Bytes
keeppool: allocate 16424 Bytes
  syncpool allocate 72 Bytes
keeppool: allocate 24636 Bytes
  syncpool deallocate 48 Bytes
  syncpool allocate 284 Bytes
  syncpool allocate 156 Bytes
  syncpool allocate 540 Bytes
  syncpool allocate 284 Bytes
  syncpool allocate 1052 Bytes
  syncpool allocate 540 Bytes
  syncpool allocate 2076 Bytes
  syncpool allocate 1052 Bytes
  syncpool allocate 4124 Bytes
  syncpool deallocate 284 Bytes
  syncpool deallocate 156 Bytes
  syncpool deallocate 540 Bytes
  syncpool deallocate 284 Bytes
  syncpool deallocate 1052 Bytes
  syncpool deallocate 2076 Bytes
  syncpool deallocate 540 Bytes
  syncpool deallocate 92 Bytes
  syncpool allocate 2076 Bytes
  syncpool allocate 8220 Bytes
  syncpool deallocate 4124 Bytes
  syncpool deallocate 1052 Bytes
--- third iteration done
--- leave scope of pool
  syncpool deallocate 2076 Bytes
  syncpool deallocate 8220 Bytes
  syncpool deallocate 16412 Bytes
  syncpool deallocate 72 Bytes
  syncpool deallocate 8 Bytes
--- leave scope of keeppool
keeppool: deallocate 24636 Bytes
keeppool: deallocate 16424 Bytes
keeppool: deallocate 10000 Bytes
