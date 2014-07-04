# README #

Monitor is a command-line tool that captures command-line input/output and sends to commands.com

Monitor makes it easy to automate set-up/install of repos such as this one. You can easily show people errors and output from commands.

EXAMPLE: Here is a [Monitor capture of installing monitor](https://commands.com/install-monitor-macosx)

You can install monitor easily by running the script from the capture:
~~~~
    Mac OS X:
    curl commands.io/install-monitor-macosx | sh
~~~~
~~~~
    Ubuntu:
    curl commands.io/install-monitor-ubuntu | sh
    Redhat:
    curl commands.io/install-monitor-redhat | sh        
~~~~
or, you can do it step-by-step...

### What is contained in this repository? ###

* monitor v 1.0
* readline

### How do I get set up? ###

* [Sign-up for an account at commands.com](https://commands.com/)

* Clone this repository
~~~~
    git clone https://dtannen@bitbucket.org/dtannen/monitor.git
~~~~

* Build, install readline
~~~~
    Mac OS X:
    cd monitor/readline
    ./configure
    make
    sudo make install
~~~~
~~~~
    Ubuntu:
    apt-get install libreadline-dev

    OR

    Redhat: 
    yum install readline-devel
~~~~

* Build monitor
~~~~
    cd monitor
    make
    sudo make install
~~~~

### Monitor Usage ###
~~~~
    Usage: monitor {-d} {-h} {-u <username>}

    -d : do not delete /tmp files
    -h : help
    -u : commands.com username
~~~~
### Contribution guidelines ###

* Please clearly describe your changes so I can test/incorporate into the main branch.

### Who do I talk to? ###

* Submit feedback at Commands.com or message me here.
