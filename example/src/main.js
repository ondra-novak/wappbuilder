//!require required1.js
//!require @custom_page custom_page.html
//


var App = function(){
	
	"use strict"

	function start() {
		console.log("Application started...!");
		if (!HashRouter.init()) {
			this.mainPage();
		}
		document.addEventListener("styles_loaded",function() {
			console.log("All defered styles has been loaded...");
		})
	};

	function mainPage() {
		console.log("Entered main page...");		
	}
	
	
	return {
		"mainPage":mainPage,
		"start":start
	};
	
	
	
}();
