#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

class nid_information
{

	private:
		int pid;
		int my_fd;
		int agent_fd;
		int udpportnum;
		int up;
		u_int64_t mesg;
		u_int16_t proc_type;
	public:
		nid_information *next;

		nid_information()
		{
			pid = 0;
			my_fd = 0;
			agent_fd = 0;
			udpportnum = 0;
			up = 0;
			mesg = 0;
			next = NULL;
		}

		void init(int pid, int my_fd, int agent_fd, int udpportnum, u_int16_t type)
		{
			this->pid = pid;
			this->my_fd = my_fd;
			this->agent_fd = agent_fd;
			this->udpportnum = udpportnum;
			this->proc_type = type;
			up = 1;
		}

		int is_up(int fd)
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(tmp->my_fd == fd)
					return tmp->up;
				tmp = tmp->next;
			}
			return 0;
		}

		int is_agent_up()
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(tmp->proc_type == PROCESS_TYPE_AGENT)
					return tmp->up;
				tmp = tmp->next;
			}
			return 0;
		}		

		void set_up(int fd, int up)
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(tmp->my_fd == fd)
					tmp->up = up;
				break;
			}
			tmp = tmp->next;
		}

		int find_pid(int pid)
		{	
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(tmp->pid == pid)
					return 1;
				tmp = tmp->next;
			}
			return -1;
		}

		int find_fd(int fd)
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(tmp->my_fd == fd)
					return 1;
				tmp = tmp->next;
			}
			return -1;
		}

		int agent_fd_pid_to_my_fd(int socket_fd, int pid)
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if((socket_fd == tmp->agent_fd) && (pid == tmp->pid))
					return tmp->my_fd;
				tmp = tmp->next;
			}
			return 0;
		}

		int* pid_to_my_fd(int pid, int &size)
		{
			int *fds, i;
			nid_information *tmp;
			tmp = this;
			fds = NULL;
			size = 0;

			while(tmp !=  NULL)
			{
				if(pid == tmp->pid)
					size++;
				tmp = tmp->next;
			}

			if(size == 0)
				return NULL;

			tmp = this;
			fds = new int[size];
			i = 0;

			while(tmp !=  NULL)
			{
				if(pid == tmp->pid)
				{
					fds[i] = tmp->my_fd;
				}
				tmp = tmp->next;
			}

			return fds;
		}

		void clear_pid_fd(int pid)
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(pid == tmp->pid)
				{
					tmp->pid = 0;
					tmp->my_fd = 0;
				}
				tmp = tmp->next;
			}
		}

		int* get_all_udpportnum(int &size)
		{
			int *udpports, i;
			nid_information *tmp;
			tmp = this;
			udpports = NULL;
			size = 0;

			while(tmp !=  NULL)
			{
				size++;
				tmp = tmp->next;
			}

			if(size == 0)
				return NULL;

			tmp = this;
			udpports = new int[size];
			i = 0;

			while(tmp !=  NULL)
			{
				udpports[i] = tmp->udpportnum;
				tmp = tmp->next;
			}

			return udpports;
		}

		void set_mesg(int fd, u_int64_t mesg)
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{	
				if(fd == tmp->my_fd)
				{
					tmp->mesg = mesg;
					break;
				}
				tmp = tmp->next;
			}
		}

		u_int64_t get_mesg(int fd)
		{
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(fd == tmp->my_fd)
					return tmp->mesg;
				tmp = tmp->next;
			}
			return 0;
		}

		void clear_all(void)
		{
			int *fds, size, i;
			nid_information *tmp;
			tmp = this;
			while(tmp !=  NULL)
			{
				if(tmp->my_fd != 0)
				{
					fds = this->pid_to_my_fd(tmp->pid, size);
					for(i=0 ; i<size ; i++)
					{
						if(fds[i] != 0)
						{
							close(fds[i]);
							if (kill(-tmp->pid, SIGKILL) >= 0) {
								printf("[closeAllClientSocket]:: kill process of pid %d\n", tmp->pid);
							} else {
								printf("[closeAllClientSocket]:: kill pid %d failed\n", tmp->pid);
							}
						}
					}
					this->clear_pid_fd(tmp->pid);
				}
				tmp = tmp->next;	
			}
		}
};
