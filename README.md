# Lab 2 - Multithreaded Chat Application

_Ryan George and Rishabh Rastogi_  
Our implementation of a C based chat application, completing all the proposed extensions and thoroughly tested.

## Development

The development process we naturally followed was iterative and prototype based, where we completed incremental prototypes and tested along the way. This can be seen in our git commits, implementing and evaluating small changes till it met the project requirements. For this project, we split the work into three: Chat Server (Ryan), Chat Client (Rishabh) and Testing (Rishabh).

The client was quite small and relatively simple, so we placed an emphasis on polishing the server so it has full functionality and has safeguards to eliminate undefined behaviour especially with NULL pointers, handling the heap and invalid requests from clients. This was done in both development and testing.

Like the first project, we extensively used git to source control our work and used Github as our remote:
![Our git commits](/Images/commits.png)
This was very useful in catching errors and seeing the progress that we both were completing. As before, any bugs that appeared first time for example, could be usually pinpointed by `git diff` with the previous commit hash (-y really helps).  
In general, we communicated great and completed both assignments within 4 weeks, leaving us time to polish/comment our code and write the README.

## Testing
