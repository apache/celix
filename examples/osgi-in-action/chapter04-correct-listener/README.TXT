In the log server a listener registers when the log_service bundle is stopped and 
started by means of the serviceChanged() routine. This routine maintains the set of log
services in the variable m_logServiceRefs. The function getLogService() returns the
current LOG_SERVICE, if it is active or NULL.
Starting and stopping the log_service bundle shows that the logServiceTest thread 
correctly recognizes the non-availability of the log_services, when stopped.
