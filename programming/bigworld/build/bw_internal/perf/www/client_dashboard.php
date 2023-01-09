<?php
if (function_exists( "gethostname" ))
{
	$hostname = gethostname();
}
else
{
	$hostname = php_uname( 'n' );
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
   "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
	<title>Client Dashboard</title>

	<meta http-equiv="refresh" content="420;URL='http://perf/client_dashboard.php'" />  

	<script type="text/javascript" src="https://www.google.com/jsapi"></script>
	<script type="text/javascript" src="jquery-1.7.2.min.js"></script>
	<script type="text/javascript" src="jquery-ui-1.8.21.custom.min.js"></script>
	<script type="text/javascript">
		google.load( "visualization", "1", {packages:["corechart"]} );				
		google.setOnLoadCallback( loadGraphs );

	 	var greyColor= '#888888';
        var darkGreyColor= '#272928';
        var yellowColor= '#eece4d';
        var blueColor= '#3e94ff';
		var redColor= '#c72b07';
			
	 	function changeBackground(color) {
        	document.body.style.background = color;
			document.getElementById("bot_bar_consumer").innerHTML =  "<font color = " + blueColor + "> &bull; Consumer";
		 	document.getElementById("bot_bar_consumer_cs").innerHTML =  "<font color = " + yellowColor + "> &bull; Consumer CS";

        }



		var CHART_TITLES = {
			"fps_regression": "Frame Time (ms)",
			"fps_absolute": "Frame Time (ms)",
			"loadtime": "Load Time (sec)",
			"peak_mem_usage": "Peak Memory Usage (MB)",
			"total_mem_allocs": "Memory Allocations"
		}
		
		function roundFloat( v )
		{
			return parseFloat( v.toFixed(2) );
		}
		
		Date.prototype.addDays = function(days) 
		{
			var dat = new Date(this.valueOf())
			dat.setDate(dat.getDate() + days);
			return dat;
		}
		
		function getDates(startDate, stopDate) 
		{
			var dateArray = new Array();
			var currentDate = startDate;
			while (currentDate <= stopDate)
			{
				dateArray.push(currentDate)
				currentDate = currentDate.addDays(1);
			}
			return dateArray;
		}
		
		function makeDate( d )
		{
			return new Date( d[2], d[1]-1, d[0] );
		}
		
		function perfDataURL( type, columns )
		{
			var url = 'perf_results_json.php?type='+type+'&';
			var count = 0;
			for (var i = 0, column; column = columns[i]; i++)
			{
				if (count > 0)
				{
					url += '&';
				}
				
				var joinedColumns = column.hostname + ',' + column.configuration + ',' + column.executable + ',' + 
									column.branch + ',' + column.benchmark + ',' + column.name;
				
				url += "column[]=" + encodeURIComponent( joinedColumns );
				
				count += 1;
			}			
		
			return url;
		}
		
		function validateColumns( columns )
		{
			if (columns.length == 0)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		
		function clearChildren( element )
		{
			if ( element.hasChildNodes() )
			{
				while (element.childNodes.length > 0)
				{
					element.removeChild( element.firstChild );
				} 
			}
		}
		
		function makeDataTable( opts,options )
		{
			var type = opts.type;
			var jsonData = opts.jsonData;
			var performanceTarget = opts.performanceTarget;
			var dataTable = new google.visualization.DataTable();
			
			// Add date column at index 0
			dataTable.addColumn( "date", "Date", "Date" );
			
			// Insert the columns (no data yet)
			// Calculate min/max vertical bounds at this point
			for (var colIdx in jsonData.columns)
			{
				colIdx = parseInt( colIdx );
				var column = jsonData.columns[colIdx];
				dataTable.addColumn( {type:"number", label:column.name, id:column.name} );

				if (column.referenceValue)
				{						
					if ( column.name == "Consumer" || column.name == "Chunk space")
					{						
						options.vAxis.baseline = column.referenceValue
						options.vAxis.baselineColor = 'red'
						if (type == "fps_absolute" )
						{
							document.getElementById("frametime_stat1").innerHTML = "<font color=" + redColor + ">" + column.referenceValue
						}
						else if ( type == "loadtime")
						{
							document.getElementById("loadtime_stat1").innerHTML = "<font color=" + redColor + ">" + column.referenceValue
						}
						else if ( type == "peak_mem_usage")
						{
							options.vAxis.baseline = roundFloat( column.referenceValue / (1024*1024) )
						}
						else if ( type == "total_mem_allocs")
						{
							options.vAxis.baseline = roundFloat( column.referenceValue  / 1000000 )
						}						
					}
				}
				if (type == "fps_regression")
				{	
					dataTable.addColumn( {type:"number", label: column.name + " (Reference)", id:column.name} );
				}
			}
			options.hAxis.maxValue = makeDate( jsonData.dateMax );
			// Add some padding on the right side of the graph
			options.hAxis.maxValue = options.hAxis.maxValue.addDays(2);
			// Insert dates, and stub out values to undefined.
			var dates = getDates( makeDate( jsonData.dateMin ), makeDate( jsonData.dateMax ) );
			for (var i = 0; i < dates.length; i++)
			{
				var stubRow = new Array();
				stubRow[0] = dates[i];
				for (var colIdx in jsonData.columns)
				{
					colIdx = parseInt( colIdx );
					var column = jsonData.columns[colIdx];
					if (type == "fps_regression")
					{
						stubRow[colIdx*2+1] = undefined;
						stubRow[colIdx*2+2] = roundFloat( column.referenceValue );
					}
					else
					{
						stubRow[colIdx+1] = undefined;
					}
				}
				
				dataTable.addRow( stubRow );
			}
			
			// Get the month, day, and year.
			var column = jsonData.columns[0];
			var row = column.data[0];
			var date = makeDate(  row[0] );
			var dateString = date.getDate() + "/";
			var last_consumer_value = 0;
			var last_cc_value = 0;
			var ref_value = column.referenceValue;
			
			dateString += (date.getMonth() + 1) + "/";
			dateString += date.getFullYear();
			document.getElementById("bot_bar_date").innerHTML = "<font color=" + greyColor + " size = '10'> since " + dateString;
			
			// Go through each column again, now populating the data.
			for (var colIdx in jsonData.columns)
			{
				colIdx = parseInt( colIdx );
				var column = jsonData.columns[colIdx];
				
				var dataIdx = colIdx+1;
				if (type == "fps_regression")
				{
					dataIdx = (colIdx*2)+1;
				}
				
				for (var rowIdx in column.data)
				{
					var row = column.data[rowIdx];
					var date = makeDate( row[0] );
					var value = 0;
						
					if (type == "fps_regression")
					{
						value = roundFloat( row[1] - column.referenceValue );
					}
					else if ( type == "fps_absolute")
					{
						value = roundFloat( row[1] );
						if ( column.name == "Consumer")	
						{
							referenceValue = column.referenceValue
							last_consumer_value = roundFloat( row[1] - referenceValue );
							document.getElementById("frametime_stat2").innerHTML = "<font color=" + blueColor + ">" + last_consumer_value;

						}
						else
						{
							last_cc_value = roundFloat( row[1] - referenceValue )
							document.getElementById("frametime_stat3").innerHTML = "<font color=" + yellowColor + ">" + last_cc_value;
						}

					}
					else if (type == "loadtime")
					{
						value = roundFloat( row[1] );
						if ( column.name == "Consumer")
                        {
							referenceValue = column.referenceValue
                            last_consumer_value = roundFloat( row[1] - referenceValue );
                            document.getElementById("loadtime_stat2").innerHTML = "<font color=" + blueColor + ">" + last_consumer_value;

                        }
                        else
                        {
                            last_cc_value = roundFloat( row[1] - referenceValue )
                            document.getElementById("loadtime_stat3").innerHTML = "<font color=" + yellowColor + ">" + last_cc_value;
                        }
					}
					else if (type == "peak_mem_usage")
					{
						value = roundFloat( row[1] / (1024*1024) );
						if ( column.name == "Chunk space")
                        {  
							last_consumer_value = value
							document.getElementById("peakMemTitle_stat2").innerHTML = "<font color=" + blueColor + ">" + last_consumer_value;
                        }
                        else
                        {
                            last_cc_value = value
							document.getElementById("peakMemTitle_stat3").innerHTML = "<font color=" + yellowColor + ">" + last_cc_value;
                        }
					}
					else if (type == "total_mem_allocs")
					{
						value = roundFloat( row[1]  / 1000000 );
						if ( column.name == "Chunk space")
                        {
                            last_consumer_value = value
							document.getElementById("totalMemTitle_stat2").innerHTML = "<font color=" + blueColor + ">" + last_consumer_value + "M";
                        }
                        else
                        {
                            last_cc_value = value
							document.getElementById("totalMemTitle_stat3").innerHTML = "<font color=" + yellowColor + ">" + last_cc_value + "M";
                        }
					}

					var diff = date.getTime() - dates[0].getTime(); // result in MS
					diff /= 1000; // in seconds
					diff /= 60; // in minutes
					diff /= 60; // in hours
					diff /= 24; // in days
					
					var dateRow = Math.floor( diff );
					
					dataTable.setCell( dateRow, dataIdx, value );
					
				}
				
				
					
			}
			
			// if any values are above the threshold, zero or diff by more than 10%, colour the title bar red
			
			var cons_percent_change = 0;
			var cc_percent_change = 0;
			
			if (type == "fps_absolute" || type == "loadtime") {
				var cons_percent_change = Math.abs((last_consumer_value / ref_value) *100);
				var cc_percent_change = Math.abs((last_cc_value / ref_value) * 100);
			} 
			
			//console.log("percent cons " + cons_percent_change);
			//console.log("percent cc "  + cc_percent_change);
			
			if (last_consumer_value > 0 || last_cc_value > 0 || cons_percent_change > 10 || cc_percent_change > 10)		
			{
				if (type == "fps_absolute")
				{
					document.getElementById('frameTimeTitle').className += ' alert';
				} 
				else if (type == "loadtime")
				{
					document.getElementById('loadTimeTitle').className += ' alert';
				}
			}
			
			if (last_consumer_value < 1 || last_cc_value < 1 )
			{
				if (type == "peak_mem_usage")
				{
					document.getElementById('peakMemTitle').className += ' alert';
				}
				else if (type == "total_mem_allocs")
				{
					document.getElementById('totalMemTitle').className += ' alert';
				}
			}
				
			var formatter = new google.visualization.DateFormat( { 'pattern': "d MMM yyyy" } )
			//console.log("Before: "+dataTable.toJSON());
			formatter.format( dataTable, 0 );
			//console.log("After: "+dataTable.toJSON());
			return dataTable;
		}

		function makeOptions( type, title )
		{	
			var options = {
				
				chartArea: { width:"100%", height:"100%",//left:20,top:0,width:"60%",height:"80%",
                                backgroundColor: {
                                        stroke: greyColor,
                                        strokeWidth: '1',
                                        gradient: {               // Attributes for linear gradient fill.
                                            color1: darkGreyColor,      // Start color for gradient.
                                            color2: 'black',      // Finish color for gradient.
                                            x1: '100%', y1: '0%',     // Where on the boundary to start and end the
                                            x2: '100%', y2: '100%', // color1/color2 gradient, relative to the
                                                                // upper left corner of the boundary.
                            }}},
				
				backgroundColor: {//fill: 'black',
								gradient: {               // Attributes for linear gradient fill.
                                            color1: '#151716',      // Start color for gradient.
                                            color2: 'black',      // Finish color for gradient.
                                            x1: '100%', y1: '0%',     // Where on the boundary to start and end the
                                            x2: '100%', y2: '50%', // color1/color2 gradient, relative to the
                                        },
								strokeWidth: '1' },
				lineWidth: '5',
				pointSize : '9',
				colors: [blueColor, yellowColor, redColor],
				//titleTextStyle: {color: '#ffffff', fontName: 'calibri', fontSize: '58'},
				vAxis: {	viewWindowMode: "explicit",
							gridlines: {color: greyColor, count: '5'},
                            textStyle: {color: greyColor, fontSize: '25'}
						},
				hAxis: {
							gridlines: {color: 'black'},
							textStyle: {color: 'black'}},
				curveType: "linear",
				title: title,
				interpolateNulls: true,
				focusTarget: "category",
				legend: { 
					position: 'none' 
				}
			};
			
				
		/*	if (type == "fps_absolute")
			{
				options.vAxis.baseline = 14;
			}
			else if (type == "loadtime")
			{
				options.vAxis.baseline = 30;
			}*/
			if (type == "total_mem_allocs")
			{
			//	 options.vAxis.baseline = 5000000;
				options.vAxis.gridlines.count = 5;
			}
			else if (type == "peak_mem_usage")
            {
              //  options.vAxis.baseline = 300;
				options.vAxis.gridlines.count = 5;
            }
			
			return options;
		}
		
		function loadGraph( opts )
		{
			var element = opts.element;
			var type = opts.type;
			var columns = opts.columns;
			var title = opts.title;
			var performanceTarget = opts.performanceTarget;
		
			var url = perfDataURL( type, columns );
			//console.log( "loadGraph", columns, url );
			
			$.getJSON( url, function(jsonData) 
			{
				//console.log("URL:" + url)
				//console.log("JSON Data: " + JSON.stringify(jsonData))
				var options = makeOptions( type, title );
				var dataTable = makeDataTable( {'type': type, 
												'jsonData':jsonData, 
												'performanceTarget':performanceTarget},
												options );
								
				var subDiv = document.createElement("div");
				subDiv.className = "chart-inner";				
				element.appendChild( subDiv );
				
				var maximizeButton = document.createElement( "div" );
				maximizeButton.className = "chart-maximize-button ui-icon ui-icon-zoomin";
				maximizeButton.onclick = function() {
					var popup = document.getElementById( 'popup-chart' );
					
					
					$( "#popup-chart" ).dialog({
						height: 600,
						width: "80%",
						modal: true,
						draggable:false,
						resizable:false,
						title: "fhqwhgads"
					});
					
					$(".ui-widget-overlay").click(function(){
						$(".ui-dialog-titlebar-close").trigger('click');
					});
					
					clearChildren( popup );
					
					var chart = new google.visualization.LineChart( popup );
					
					chart.draw( dataTable, options );
				};
				
				element.appendChild( maximizeButton );
				
				// All done, spawn the chart
				var chart = new google.visualization.LineChart( subDiv );
				chart.draw( dataTable, options );
				
			} );
		}
		
		function loadGraphs()
		{
			$(".chart").each( function()
			{
				var columns = [];
				
				var title = $(this).attr( "title" );
				var type = $(this).attr( "type" );
				var performanceTarget = $(this).attr( "performanceTarget" );
				if (type == undefined)
				{
					type = "fps_regression";
				}
				
				//console.log( "blah", type );
				
				$(this).find(".column").each( function() 
				{
					columns.push( {
						hostname: $(this).attr( "hostname" ),
						configuration: $(this).attr( "configuration" ),
						executable: $(this).attr( "executable" ),
						branch: $(this).attr( "branch" ),
						benchmark: $(this).attr( "benchmark" ),
						name: $(this).attr( "name" )
					} );
				} );
				if (validateColumns( columns ))
				{
					loadGraph( {'element':this, 
								'type':type, 
								'columns':columns, 
								'title':title, 
								'performanceTarget':performanceTarget} );
				}
				else
				{
					$(this).innerHTML = "<div class='chart-error'>Invalid columns: " + columns + "</div>";
				}
				
				
			} );
		}
		
	</script>
	
	<style>
	<!--
	body
	{
		font-family: Arial;
		height:1080px;
		width:1920px;
		
	}
	
	div.chart
	{
		
	}
	
	div.chart-inner
	{
		height: 100%;
	}
	
	div.chart-maximize-button
	{
		position:absolute;
		top:0;
		right:0;		
		cursor:pointer;
	}
	
	div.chart-category
	{
		clear: both;
	}
	
	div.frames
	{
		text-align:left;
		font-size:52px;
		color:white;
		font-weight:bold;
        background-color:black;
        border: 1px solid #888888;
		padding-bottom:6px;
	}
	
	div.rightbar
	{
		border-right: 5px solid #333;
	}
	div.leftbar
	{
		border-left: 5px solid #555;
	}
	div.topbar
	{
		border-top: 5px solid #333;
	}
	div.bottombar
	{
		border-bottom: 5px solid #555;
	}
	
	div.frames.alert
	{
		border: 1px solid #FF0000;
		background-color:rgba(255,0,0,0.3);
	}
	
	.ui-resizable-helper
	{
		border: 1px dotted #00F;
	}
	
	#blanket 
	{
		background-color:#111;
		opacity: 0.65;
		position:absolute;
		z-index: 9001; /*ooveeerrrr nine thoussaaaannnd*/
		top:2px;
		left:2px;
		width:100%;
	}
	
	.ui-dialog .ui-dialog-content
	{
		padding:0;
	}
	
	.ui-dialog-titlebar 
	{ 
		display: none; 
	}
	-->
	</style>
</head>

<body onload="changeBackground('black');">
	<div class="chart-category">
		<div style="width:100%;height: 1014px;">
			<div class="rightbar bottombar" style="width: 49%; float: left;">
				<div class ="frames" style="width:99.8%;height:52px" id="frameTimeTitle">
					&nbsp;Frame Time
					<span style="padding-left:100px" id="frametime_stat1">0</span>
					<span style="padding-left:10px" id="frametime_stat2">0</span>
					<span style="padding-left:10px" id="frametime_stat3">0</span>
					<font size="6"><b>
					<span class="units">ms</span>
				</div> 
				
				<div class="chart bottombar" style="width:100%;height:440px" title="Frame Time" type="fps_absolute" performanceTarget="16">
					<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerRuinberg" name="Consumer"></div>
					<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerRuinberg" name="Compiled space - Consumer"></div>
					 <!-- <div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_7_full_2.7_BW" benchmark="RunProfilerHighlands" name="laaaa"></div> -->
				</div>
							  
				<div class ="frames topbar" style="width:99.8%; height:52px" id="totalMemTitle">
					&nbsp;Memory Allocation
					<!-- <span style="padding-left:100px" id="totalMemTitle_stat1">0</span>-->
					<span style="padding-left:75px" id="totalMemTitle_stat2">0</span>
					<span style="padding-left:10px" id="totalMemTitle_stat3">0</span>
					<font size="6"><b>
				</div>
				<div class="chart" style="width:100%;height:440px" title="Mem Alloc" type="total_mem_allocs">
					<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10 (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Chunk space"></div>
					<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Compiled space"></div>
				</div>
				
			</div>
		
			<div class="leftbar bottombar" style="width: 49%; float: left;">
				
				<div class ="frames" style="width:99.8%;height:52px" id="loadTimeTitle">
					&nbsp;Load Time
					<span style="padding-left:100px" id="loadtime_stat1">0</span>
					<span style="padding-left:10px" id="loadtime_stat2">0</span>
					<span style="padding-left:10px" id="loadtime_stat3">0</span>
					<font size="6"><b>
					<span class="units">sec</span>
				</div>

				 <div class="chart bottombar" style="width:100%;height:440px" title="Load Time" type="loadtime">
					<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="spaces/08_ruinberg" name="Consumer"></div>
					<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/08_ruinberg" name="Compiled space - Consumer"></div>
				</div>
				
				<div class ="frames topbar" style="width:99.8%;height:52px" id="peakMemTitle">
					&nbsp;Memory Peak
					<!-- <span style="padding-left:100px" id="peakMemTitle_stat1">0</span>-->
					<span style="padding-left:75px" id="peakMemTitle_stat2">0</span>
					<span style="padding-left:10px" id="peakMemTitle_stat3">0</span>
					<font size="6"><b>
					<span class="units">MB</span>
				</div>
				
				<div class="chart" style="width:100%;height:440px" title="Mem Peak" type="peak_mem_usage">
					<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10 (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Chunk space"></div>
					<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Compiled space"></div>
				</div>
			</div>
		</div>
		<div class ="frames topbar" style="width:98.5%;height:44px; padding-bottom:16px;">
			<span id="bot_bar_title"> &nbsp;Ruinberg 2.10 </span>
			<span id="bot_bar_date"  style="padding-left:150px">  </span>
			<span id="bot_bar_consumer" style="padding-left:200px"> &bull; Consumer </span>&nbsp;
			<span id="bot_bar_consumer_cs" style="padding-left:50px"> &bull; Consumer CS </span>
        </div>
		
	</div>
		
</body>
</html>
