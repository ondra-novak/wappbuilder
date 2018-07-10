//!require required1.js


var App = function(){
	
	"use strict"

	function start() {
		console.log("Application started...!");
		if (!HashRouter.init()) {
			this.mainPage();
		}
	};

	function mainPage() {
		console.log("Entered main page...");		
	}
	
	
	return {
		"mainPage":mainPage,
		"start":start
	};
	
	
	
}();
