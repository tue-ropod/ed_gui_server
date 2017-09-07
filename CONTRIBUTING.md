Build Setup
====================

[Grunt](http://gruntjs.com/) is used for building, including concatenating, minimizing, documenting, linting, and testing.

[Bower](http://bower.io/) is used for downloading and managing client-side needed js libraries (e.g. JQuery).

### Install Grunt, Bower and their Dependencies

 1. Install Node.js and its package manager, NPM
   * `wget -qO- https://deb.nodesource.com/setup | bash -`
   * `sudo apt-get install nodejs`
 2. Install Grunt and Bower
   * `sudo -H npm install -g grunt-cli bower`
 3. Install other dependencies
   * `sudo apt-get install libgif-dev`
 4. For the gui, install its dependencies
   * `roscd ed_gui_server/html`
   * `npm install`
   * `bower install`

### Build with Grunt

Before proceeding, please confirm you have installed the dependencies above.

To run the build tasks:

 1. `roscd ed_gui_server/html`
 2. `grunt build`

`grunt build` will build the app in the dist/ directory. It will also run the linter and test cases.

`grunt serve` will watch for any changes to any of the app/ files and automatically build the files. This is ideal for those developing as you should only have to run `grunt serve` once.
