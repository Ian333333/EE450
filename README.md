# EE450 Socket Project

Name: Yimin Chen

Student ID: 9771232431

------

What I have done: 

1. Phase 1 to Phase 4 including following operations:

   - CHECK WALLET(./clientA Ali)
   - TXCOINS(./clientA Ali Racheal 100)
   - TXLIST(./clientA TXLIST)
   - stats(./clientA Ali stats)

2. What my code files do:

   - serverA: read data from block1.txt and write data into block1.txt
   - serverB: read data from block2.txt and write data into block2.txt
   - serverC: read data from block3.txt and write data into block3.txt
   - serverM: when clientA/B send request to serverM, serverM firstly request log entry from serverA, serverB and serverC. ServerM does all the data processing including transaction amount computing, filtering data by username, sorting and so on.
   - clientA: send request to serverM
   - clientB: send request to serverM

3. Since data transferred between two hosts is in bytes, the format of all the messages exchange is char array. (Although I use the format of int somewhere in the code)

4. Since accept() is blocking, in order to enable serverM to communicate with both clientA and clientB at the same time, I use multithread. For each of clientA and clientB, serverM create a thread which builds tcp connection with the corresponding client.

   If the input is strictly correct, there should not be failure when running my project. If it fails, just try again and it will finally success! If it still fails, close all the servers and try again!

5. I didn't use any code except the basic function of socket like create(), bind(), listen(). We can find the code if we google.