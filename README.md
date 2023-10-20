# Program-Autograding-Server-DECS
step-by-step construction of a scalable program autograding server and its client
We assume that the purpose of the submitted program is simply to print the first ten
numbers:
1 2 3 4 5 6 7 8 9 10
If the submitted program prints this output, it has passed, else it has failed.
In all versions, the server will always be run as follows,
$./server <port>
and the client will always be run as follows,
$./submit <serverIP:port> <sourceCodeFileTobeGraded> <OtherArgumentsifAny>
and will get back one of the following responses from the server:
1. PASS
2. COMPILER ERROR
3. RUNTIME ERROR
4. OUTPUT ERROR
In cases 2,3,4, the server should additionally send back the error details:
● For compiler error, the entire compiler output should be sent back to the client
● For runtime error, the error type should be sent back to the client
● For output error, the output that the program produced, and the output of a ‘diff’
command should be sent back to the client
