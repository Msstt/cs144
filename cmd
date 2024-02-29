cmake --build build

13/17 Test #14: reassembler_overlapping ..........***Failed    0.02 sec

The test "overlapping unassembled section 2" failed after these steps:

  0.    Initialized Reassembler with capacity=1000
  1.    Action: insert "c" @ index 2
  2.    Expectation: [buffer is empty] = true
  3.    Action: insert "bcd" @ index 1
  4.    Expectation: [buffer is empty] = true
  ***** Unsuccessful Expectation: bytes_pending = 3 *****

ExpectationViolation: The object should have had bytes_pending = 3, but instead it was 2.
