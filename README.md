# qtapplications
qtapplication to get the client data and copy to the pendrive 
**Project Overview:**

This project involves detecting and interacting with a pendrive on a Linux-based system through a Qt application. The primary tasks include finding user information, identifying pendrives, searching for specific logs created in the last 7 days, and transferring those logs. The application uses several system-level commands executed via `QProcess` to handle user detection, pendrive identification, and file operations.

**Project Flow (Short Overview):**

1. **Retrieve Username:**
   - Use `getCurrentUsername()` to retrieve the current system user via the `whoami` or similar command.
   
2. **Pendrive Detection:**
   - Detect connected pendrives under the `/media` directory using `detectPendrive()`, searching for mounted media for the current user.

3. **Log File Search:**
   - Using the `find` command, the application searches for log files modified within the last 7 days across multiple log directories (`coreLog`, `eventLogs`, `QtLog`).

4. **Log Processing:**
   - The found log files are processed, stored in a temporary directory, and prepared for transfer.

5. **File Transfer:**
   - Transfer the logs to a detected pendrive location using system commands executed within the Qt application.

**Key Components:**
- **Username Management:** Retrieves and uses the current user's name to ensure correct paths.
- **Pendrive Detection:** Searches through `/media` for mounted devices associated with the user.
- **Log File Search:** Uses `find` to locate and print logs within 7 days.
- **File Handling & Transfer:** Manages log files and transfers them to a pendrive.

This flow automates the detection of a pendrive and logs file processing in a simple and efficient manner using system utilities integrated within a Qt-based interface.
