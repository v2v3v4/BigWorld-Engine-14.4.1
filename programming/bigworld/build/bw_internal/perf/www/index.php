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
	<link rel="stylesheet" href="themes/ui-lightness/jquery.ui.all.css">
	<script type="text/javascript" src="https://www.google.com/jsapi"></script>
	<script type="text/javascript" src="jquery-1.7.2.min.js"></script>
	<script type="text/javascript" src="jquery-ui-1.8.21.custom.min.js"></script>
	<script type="text/javascript">
		"use strict";
		google.load( "visualization", "1", {packages:["corechart"]} );				
		google.setOnLoadCallback( loadGraphs );
		
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
		
		function makeDataTable( opts )
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
					
				if (type == "fps_regression")
				{
					dataTable.addColumn( {type:"number", label: column.name + " (Reference)", id:column.name} );
				}
			}
			
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
					else if (type == "loadtime" || type == "fps_absolute")
					{
						value = roundFloat( row[1] );
					}
					else if (type == "peak_mem_usage")
					{
						value = roundFloat( row[1] / (1024*1024) );
					}
					else if (type == "total_mem_allocs")
					{
						value = roundFloat( row[1] );
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
			console.log(type);
			var formatter = new google.visualization.DateFormat( { 'pattern': "d MMM yyyy" } )
			console.log("Before: "+dataTable.toJSON());
			formatter.format( dataTable, 0 );
			console.log("After: "+dataTable.toJSON());
			return dataTable;
		}
		
		function makeOptions( type, title )
		{
			var options = {
				curveType: "linear",
				title: title,
				pointSize : 7,
				interpolateNulls: true,
				focusTarget: "category",
				vAxis: { 
					title: CHART_TITLES[type],
					viewWindowMode: "explicit",
					viewWindow: {
						max: +5,
						min: -5
					}
				},
				hAxis: { 
					title: "Test Date",
					textStyle: {
						fontSize: 8
					} 
				},
				chartArea: {
					width: "80%", 
					height: "70%" 
				},
				legend: { 
					position: 'bottom' 
				}
			};
			
			if (type == "fps_regression")
			{
				options.series = [
						{ color: 'blue', visibleInLegend: true, lineWidth:2 },
						{ color: 'blue', visibleInLegend: false, lineWidth:1 },
						{ color: 'green', visibleInLegend: true, lineWidth:2 },
						{ color: 'green', visibleInLegend: false, lineWidth:1 },
						{ color: 'red', visibleInLegend: true, lineWidth:1 },
						{ color: 'red', visibleInLegend: false, lineWidth:1 }
					];
			}
			
			if (type == "fps_regression")
			{
				options.vAxis.viewWindowMode = "explicit";					
			}
			else
			{
				options.vAxis.viewWindowMode = "pretty";
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
			console.log( "loadGraph", columns, url );
			
			$.getJSON( url, function(jsonData) 
			{
				console.log("URL:" + url)
				console.log("JSON Data: " + JSON.stringify(jsonData))
				var dataTable = makeDataTable( {'type': type, 
												'jsonData':jsonData, 
												'performanceTarget':performanceTarget} );
				var options = makeOptions( type, title );
								
				var subDiv = document.createElement("div");
				subDiv.className = "chart-inner";				
				element.appendChild( subDiv );
				
				var maximizeButton = document.createElement( "div" );
				maximizeButton.className = "chart-maximize-button ui-icon ui-icon-zoomin";
				maximizeButton.onclick = function() {
					var popup = document.getElementById( 'popup-chart' );
					
					/*
					blanket_size( 'popup-chart' );
					toggleDisplay( 'blanket' );
					toggleDisplay( 'popup-chart' );
					*/
					
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
				
				/*
				$(element).bind( "resizestop", function() {
						var dataTable = google.visualization.arrayToDataTable( data );
						chart.draw( dataTable, options );
					} );
				
				$(element).resizable( {
					helper: "ui-resizable-helper"
				} );
				*/
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
				
				console.log( "blah", type );
				
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
	}
	
	div.chart
	{
		position:relative;
		background-image:url('ajax-loader.gif');
		background-repeat: no-repeat;
		background-position: center center;
		border: solid silver 1px;
		margin: 1em;
		padding-bottom: 20px;
		padding-right: 20px;
		
		width: 600px; 
		height: 200px;
		
		float: left;
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
		border-bottom: solid black 2px;
	}
	
	.ui-resizable-helper
	{
		border: 2px dotted #00F;
	}
	
	#blanket 
	{
		background-color:#111;
		opacity: 0.65;
		position:absolute;
		z-index: 9001; /*ooveeerrrr nine thoussaaaannnd*/
		top:0px;
		left:0px;
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

<body>
	<div id="blanket" style="display:none;"></div>
	<div id="popup-chart" style="display:none;">		
	</div>	
	
	<h1>Current Target</h1>
	<p>
	<ul>
		<li>This is our main performance target, is graphed in absolute performance values</li>
		<li>Zero in the graph are days on which the test failed to run.</li>		
	</ul>
	</p>
	<div class="chart-category">
		<div class="chart" style="width:1500px;height:600px" title="2.10 Highlands Nightly Build" type="fps_absolute" performanceTarget="16">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerHighlands" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerHighlands" name="Compiled space - Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerHighlands" name="Compiled space - Release"></div>
		</div>
	</div>
	
	<div style="clear:both"></div>
	
	<div class="chart-category">
		<h1>2.10 1920x1200(Machine: TESTING02)</h1>
		<div class="chart" title="2.10 Highlands" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerHighlands" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerHighlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerHighlands" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.10 Minspec" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerMinspec" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerMinspec" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerMinspec" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerMinspec" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.10 Ruinberg" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerRuinberg" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="RunProfilerRuinberg" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerRuinberg" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="RunProfilerRuinberg" name="Compiled space - Consumer"></div>    
		</div>

	</div>
	
	<div class="chart-category">
		<h1>2.10 640x480 (Machine: TESTING02)</h1>
		<div class="chart" title="2.10 Highlands" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_640x480" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_640x480" benchmark="RunProfilerHighlands" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_640x480" benchmark="RunProfilerHighlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_640x480" benchmark="RunProfilerHighlands" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.10 Minspec" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_640x480" benchmark="RunProfilerMinspec" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_640x480" benchmark="RunProfilerMinspec" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_640x480" benchmark="RunProfilerMinspec" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_640x480" benchmark="RunProfilerMinspec" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.10 Ruinberg" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_640x480" benchmark="RunProfilerRuinberg" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_640x480" benchmark="RunProfilerRuinberg" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_640x480" benchmark="RunProfilerRuinberg" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_640x480" benchmark="RunProfilerRuinberg" name="Compiled space - Consumer"></div>    
		</div>

	</div>
	
	
	<div class="chart-category">
		<h1>2.9 1920x1200(Machine: TESTING02)</h1>
		<div class="chart" title="2.9 Highlands" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="RunProfilerHighlands" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="RunProfilerHighlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="RunProfilerHighlands" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.9 Minspec" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="RunProfilerMinspec" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="RunProfilerMinspec" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="RunProfilerMinspec" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="RunProfilerMinspec" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.9 Ruinberg" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="RunProfilerRuinberg" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="RunProfilerRuinberg" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="RunProfilerRuinberg" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="RunProfilerRuinberg" name="Compiled space - Consumer"></div>    
		</div>

	</div>
	
	<div class="chart-category">
		<h1>2.9 640x480 (Machine: TESTING02)</h1>
		<div class="chart" title="2.9 Highlands" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW640x480" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW640x480" benchmark="RunProfilerHighlands" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="RunProfilerHighlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="RunProfilerHighlands" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.9 Minspec" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW640x480" benchmark="RunProfilerMinspec" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW640x480" benchmark="RunProfilerMinspec" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="RunProfilerMinspec" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="RunProfilerMinspec" name="Compiled space - Consumer"></div>
		</div>

		<div class="chart" title="2.9 Ruinberg" type="fps_absolute">
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW640x480" benchmark="RunProfilerRuinberg" name="Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW640x480" benchmark="RunProfilerRuinberg" name="Consumer"></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="RunProfilerRuinberg" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING02" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="RunProfilerRuinberg" name="Compiled space - Consumer"></div>    
		</div>

	</div>
	
	
	<div class="chart-category">
		<h1>Space Load Speed Tests (Machine: TESTING04)</h1>
		
		<div class="chart" title="2.7 Highlands" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_7_full_2.7_BW" benchmark="spaces/highlands" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_7_full_2.7_BW" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_7_compiled_space_2.7_BW" benchmark="spaces/highlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_7_compiled_space_2.7_BW" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.8 Highlands" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_8_full_2.8_BW" benchmark="spaces/highlands" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_8_full_2.8_BW" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_8_compiled_space_2.8_BW" benchmark="spaces/highlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_8_compiled_space_2.8_BW" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
				
		<div class="chart" title="2.9 Highlands 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="spaces/highlands" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW1920x1200" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="spaces/highlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW1920x1200" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
				
		<div class="chart" title="2.9 Highlands 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_9_full_2.9_BW640x480" benchmark="spaces/highlands" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW640x480" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="spaces/highlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW640x480" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
				
				
		<div class="chart" title="2.10 Highlands 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="spaces/highlands" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/highlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.10 Ruinberg 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="spaces/08_ruinberg" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="spaces/08_ruinberg" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/08_ruinberg" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/08_ruinberg" name="Compiled space - Consumer"/></div>
		</div>

		<div class="chart" title="2.10 Minspec 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="spaces/minspec" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="spaces/minspec" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/minspec" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/minspec" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.10 Arctic 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_1920x1200" benchmark="spaces/arctic" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_1920x1200" benchmark="spaces/arctic" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/arctic" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_1920x1200" benchmark="spaces/arctic" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.10 Highlands 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_640x480" benchmark="spaces/highlands" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_640x480" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/highlands" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.10 Ruinberg 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_640x480" benchmark="spaces/08_ruinberg" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_640x480" benchmark="spaces/08_ruinberg" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/08_ruinberg" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/08_ruinberg" name="Compiled space - Consumer"/></div>
		</div>

		<div class="chart" title="2.10 Minspec 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_640x480" benchmark="spaces/minspec" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_640x480" benchmark="spaces/minspec" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/minspec" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/minspec" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.10 Arctic 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_full_640x480" benchmark="spaces/arctic" name="Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_640x480" benchmark="spaces/arctic" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/arctic" name="Compiled space - Release"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_640x480" benchmark="spaces/arctic" name="Compiled space - Consumer"/></div>
		</div>
	</div>
	
	
	
	
	<div class="chart-category">
		<h1>Primed cache Space Load Speed Tests (Machine: TESTING04)</h1>		
				
		<div class="chart" title="2.9 Highlands 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW_primedCacheLoadTime1920x1200" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW_primedCacheLoadTime1920x1200" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.9 Ruinberg 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW_primedCacheLoadTime1920x1200" benchmark="spaces/08_ruinberg" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW_primedCacheLoadTime1920x1200" benchmark="spaces/08_ruinberg" name="Compiled space - Consumer"/></div>
		</div>

			<div class="chart" title="2.9 Highlands 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW_primedCacheLoadTime640x480" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW_primedCacheLoadTime640x480" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.9 Ruinberg 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_full_2.9_BW_primedCacheLoadTime640x480" benchmark="spaces/08_ruinberg" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_9_compiled_space_2.9_BW_primedCacheLoadTime640x480" benchmark="spaces/08_ruinberg" name="Compiled space - Consumer"/></div>
		</div>
			
		
		<div class="chart" title="2.10 Highlands 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_primedCacheLoadTime1920x1200" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_primedCacheLoadTime1920x1200" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.10 Ruinberg 1920x1200" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_primedCacheLoadTime1920x1200" benchmark="spaces/08_ruinberg" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_primedCacheLoadTime1920x1200" benchmark="spaces/08_ruinberg" name="Compiled space - Consumer"/></div>
		</div>

			<div class="chart" title="2.10 Highlands 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_primedCacheLoadTime640x480" benchmark="spaces/highlands" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_primedCacheLoadTime640x480" benchmark="spaces/highlands" name="Compiled space - Consumer"/></div>
		</div>
		
		<div class="chart" title="2.10 Ruinberg 640x480" type="loadtime">
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_full_primedCacheLoadTime640x480" benchmark="spaces/08_ruinberg" name="Consumer"/></div>
			<div class="column" hostname="TESTING04" configuration="WIN32" executable="CONSUMER" branch="bw_2_10_compiled_space_primedCacheLoadTime640x480" benchmark="spaces/08_ruinberg" name="Compiled space - Consumer"/></div>
		</div>
	</div>
	
	<div class="chart-category">
		<h1>Smoke Test Peak Memory Usage (Machine: TESTING03)</h1>
		<div class="chart" title="2.10 Highlands" type="peak_mem_usage">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10 (WIN32)" benchmark="bwclientsmoketest_highlands" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space (WIN32)" benchmark="bwclientsmoketest_highlands" name="Compiled space"/></div>
		</div>
		
		<div class="chart" title="2.9 Highlands" type="peak_mem_usage">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="2.9_BW (WIN32)" benchmark="bwclientsmoketest_highlands" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="compiled_space_2.9_BW (WIN32)" benchmark="bwclientsmoketest_highlands" name="Compiled space"/></div>
		</div>
		
		<div class="chart" title="2.10 Ruinberg" type="peak_mem_usage">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10 (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Compiled space"/></div>
		</div>
		
		<div class="chart" title="2.9 Ruinberg" type="peak_mem_usage">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="2.9_BW (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="compiled_space_2.9_BW (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Compiled space"/></div>
		</div>
		
	</div>
		
	<div class="chart-category">
		<h1>Smoke Test Total Memory Allocations (Machine: TESTING03)</h1>
		<div class="chart" title="2.10 Highlands" type="total_mem_allocs">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10 (WIN32)" benchmark="bwclientsmoketest_highlands" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space (WIN32)" benchmark="bwclientsmoketest_highlands" name="Compiled space"/></div>
		</div>
		
		<div class="chart" title="2.9 Highlands" type="total_mem_allocs">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="2.9_BW (WIN32)" benchmark="bwclientsmoketest_highlands" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="compiled_space_2.9_BW (WIN32)" benchmark="bwclientsmoketest_highlands" name="Compiled space"/></div>
		</div>
		
		<div class="chart" title="2.10 Ruinberg" type="total_mem_allocs">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10 (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="bw_2_10_compiled_space (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Compiled space"/></div>
		</div>
		
		<div class="chart" title="2.9 Ruinberg" type="total_mem_allocs">
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="2.9_BW (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Chunk space"/></div>
			<div class="column" hostname="TESTING03" configuration="WIN32" executable="RELEASE" branch="compiled_space_2.9_BW (WIN32)" benchmark="bwclientsmoketest_ruinberg" name="Compiled space"/></div>
		</div>
		</div>
	</div>
						
	<div style="clear:both"></div>
	<p>
		<small>[Page generated by <?php echo $hostname?>:<?php echo __FILE__;?>]</small>
	</p>
</body>
</html>
