	ExecTrace (Real-Time Code Execution Profiler)

Student Name: Talha Bin Tahir
Roll Number: BSAI24079

CURRENT STATUS:
The Backend Core is 100% functional and meets the requirements for the custom DBMS.

1. Storage Engine: 
   - Custom B-Tree implementation.
   - Supports Insertion, Search, Range Queries.
   - Fully persistent Disk I/O.

2. Indexing System (Completed):
   - Implemented Multi-Index architecture.
   - Primary Index: Function IDs.
   - Secondary Indexes: Query by RAM Usage and Execution Duration.

3. API Layer (Completed):
   - Integrated `cpp-httplib` Web Server.
   - Exposes REST endpoints (`POST /log`, `GET /query`) for external clients.

4. Hashing (Completed):
   - Implemented Polynomial Rolling Hash to auto-generate unique IDs from function names.

REMAINING TASKS (To be completed for Final Submission):

1. Frontend Dashboard: 
   - Develop React.js visualization to display real-time charts of the API data.

2. Performance Optimization:
   - Implement Circular Queue (for write buffering).

3. Advanced Analytics:
   - Implement Call Graph visualization (Adjacency List).