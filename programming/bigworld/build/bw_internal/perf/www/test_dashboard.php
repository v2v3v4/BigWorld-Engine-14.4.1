<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
   "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
	<link rel="stylesheet" href="jquery-ui/themes/base/jquery.ui.all.css">
	<script type="text/javascript" src="https://www.google.com/jsapi"></script>
	<script type="text/javascript" src="jquery-1.7.2.min.js"></script>
	<script type="text/javascript" src="jquery-ui-1.8.21.custom.min.js"></script>
	<script type="text/javascript">
		"use strict";
		google.load( "visualization", "1", {packages:["corechart"]} );				
		google.setOnLoadCallback( loadGraphs );
		
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
			return new Date( d[2], d[1], d[0] );
		}
		
		function perfDataURL( columns )
		{
			console.log( "top", columns );
			var url = 'perf_results_json.php?';
			var count = 0;
			for (var i = 0, column; column = columns[i]; i++)
			{
				console.log( "here", column );
				if (count > 0)
				{
					url += '&';
				}
				
				var joinedColumns = column.hostname + ',' + column.executable + ',' + 
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
		
		function loadGraph( element, columns, title )
		{
			var url = perfDataURL( columns );
			console.log( "loadGraph", columns, url );
			
			$.getJSON( url, function(data) {
				clearChildren( element );
				
				var options = {
					curveType: "linear",
					title: title,
					interpolateNulls: false,
					focusTarget: "category",
					vAxis: { 
						title: "Frame Time (ms)",
						viewWindowMode: "explicit",
						viewWindow: {
							max: 100,
							min: 0
						}
					},
					hAxis: { 
						title: "Test Date",
						slantedTextAngle: 90,
						showTextEvery:5,
						textStyle: { 
							fontSize: 8
						} 
					},
					chartArea: {width: "80%", height: "70%" },
					legend: { position: 'bottom' },
					series: [
							{ color: 'blue', visibleInLegend: true, lineWidth:3 },
							{ color: 'blue', visibleInLegend: false, lineWidth:1 },
							{ color: 'green', visibleInLegend: true, lineWidth:3 },
							{ color: 'green', visibleInLegend: false, lineWidth:1 },
							{ color: 'red', visibleInLegend: true, lineWidth:1 },
							{ color: 'red', visibleInLegend: false, lineWidth:1 }
						]
				};
				
				/*
				var dataTable = google.visualization.arrayToDataTable( data );
				*/
				
				var dataTable = new google.visualization.DataTable();
				
				// Add date column at index 0
				dataTable.addColumn( "date", "Date", "Date" );
				
				// Insert the columns (no data yet)
				// Calculate min/max vertical bounds at this point
				var numDataColumns = 0;				
				var minVertical = 10000;
				var maxVertical = 0;
				for (var colIdx in data.columns)
				{
					var column = data.columns[colIdx];
					dataTable.addColumn( {type:"number", label:column.name, id:column.name} );
					dataTable.addColumn( {type:"number", label: column.name + " (Reference)", id:column.name} );
					numDataColumns += 2;
					
					if (column.mean - column.stdv < minVertical)
					{
						minVertical = column.mean - column.stdv;
					}
					
					if (column.mean + column.stdv*0.5 > maxVertical)
					{
						maxVertical = column.mean + column.stdv*0.5;
					}
				}
				
				// Insert dates, and stub out values to undefined.
				var dates = getDates( makeDate( data.dateMin ), makeDate( data.dateMax ) );
				for (var i = 0; i < dates.length; i++)
				{
					var stubRow = new Array();
					stubRow[0] = dates[i];
					for (var colIdx in data.columns)
					{
						var column = data.columns[colIdx];
						stubRow[colIdx*2+1] = undefined;
						stubRow[colIdx*2+2] = column.referenceValue;
					}
					
					dataTable.addRow( stubRow );
				}
				
				// Go through each column again, now populating the data.
				for (var colIdx in data.columns)
				{
					var column = data.columns[colIdx];
					console.log ( "column index", colIdx, column.data );
					
					var dataIdx = (colIdx*2)+1;
					
					for (var rowIdx in column.data)
					{
						var row = column.data[rowIdx];
						var date = makeDate( row[0] );
						var value = row[1];
						
						var diff = date.getTime() - dates[0].getTime(); // result in MS
						diff /= 1000; // in seconds
						diff /= 60; // in minutes
						diff /= 60; // in hours
						diff /= 24; // in days
						
						var dateRow = diff;
						
						console.log( "setting", dateRow, dataIdx, value, "on column", colIdx, column.name );
						dataTable.setCell( dateRow, dataIdx, value );
					}
				}
				
				// Finalise options
				options.vAxis.viewWindow.min = minVertical;
				options.vAxis.viewWindow.max = maxVertical;
				
				// All done, spawn the chart
				var dashboard = new google.visualization.Dashboard( element );
				
				var donutRangeSlider = new google.visualization.ControlWrapper({
					  'controlType': 'NumberRangeFilter',
					  'containerId': 'filter_div',
					  'options': {
						'filterColumnLabel': 'Donuts eaten'
					  }
					});
					
				var pieChart = new google.visualization.ChartWrapper({
				  'chartType': 'LineChart',
				  'containerId': 'chart_div',
				  'options': {
					'width': 300,
					'height': 300,
					'pieSliceText': 'value',
					'legend': 'right'
				  }
				});

				// Establish dependencies, declaring that 'filter' drives 'pieChart',
				// so that the pie chart will only display entries that are let through
				// given the chosen slider range.
				dashboard.bind(donutRangeSlider, pieChart);
					
				chart.draw( dataTable );
		
				//var chart = new google.visualization.LineChart( element );
				//chart.draw( dataTable, options );
				
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
				
				$(this).find(".column").each( function() 
				{
					columns.push( {
						hostname: $(this).attr( "hostname" ),
						executable: $(this).attr( "executable" ),
						branch: $(this).attr( "branch" ),
						benchmark: $(this).attr( "benchmark" ),
						name: $(this).attr( "name" )
					} );
				} );
				
				if (validateColumns( columns ))
				{
					loadGraph( this, columns, title );
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
	div.chart
	{
		background-image:url('ajax-loader.gif');
		background-repeat: no-repeat;
		background-position: center center;
		border: solid silver 1px;
		margin: 1em;
		padding-bottom: 20px;
		padding-right: 20px;
		
		width: 1200px; 
		height: 400px;
		
	}
	
	div.chart-inner
	{
		border: solid red 1px;
		width: 100%;
		height: 100%;
	}
	
	div.chart-category
	{
		clear: both;
		border-bottom: solid black 2px;
	}
	
	.ui-resizable-helper { border: 2px dotted #00F; }
	-->
	</style>
</head>

<body>
	<h1>Bestest engineered performance graphs</h1>

	<div class="chart-category">
		<h1>3.0 (testing01, full)</h1>
		<div class="chart" title="3.0 Minspec">
			<div class="column" hostname="TESTING01" executable="RELEASE" branch="bw_3_0_full" benchmark="RunProfilerMinspec" name="Release"/></div>
			<div class="column" hostname="TESTING01" executable="CONSUMER" branch="bw_3_0_full" benchmark="RunProfilerMinspec" name="Consumer"></div>
		</div>
		
		<div class="chart" title="3.0 Highlands">
			<div class="column" hostname="TESTING01" executable="RELEASE" branch="bw_3_0_full" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING01" executable="CONSUMER" branch="bw_3_0_full" benchmark="RunProfilerHighlands" name="Consumer"></div>
		</div>
		
		<div class="chart" title="3.0 Urban">
			<div class="column" hostname="TESTING01" executable="RELEASE" branch="bw_3_0_full" benchmark="RunProfilerUrban" name="Release"/></div>
			<div class="column" hostname="TESTING01" executable="CONSUMER" branch="bw_3_0_full" benchmark="RunProfilerUrban" name="Consumer"></div>
		</div>
		
		<div class="chart" title="3.0 Wales">
			<div class="column" hostname="TESTING01" executable="RELEASE" branch="bw_3_0_full" benchmark="RunProfilerWales" name="Release"/></div>
			<div class="column" hostname="TESTING01" executable="CONSUMER" branch="bw_3_0_full" benchmark="RunProfilerWales" name="Consumer"></div>
		</div>
	</div>

	<div class="chart-category">
		<h1>2.1 (testing02, full)</h1>
		<div class="chart" title="2.1 Minspec">
			<div class="column" hostname="TESTING02" executable="RELEASE" branch="bw_2_1_full" benchmark="RunProfilerMinspec" name="Release"/></div>
			<div class="column" hostname="TESTING02" executable="CONSUMER" branch="bw_2_1_full" benchmark="RunProfilerMinspec" name="Consumer"></div>
		</div>
		
		<div class="chart" title="2.1 Highlands">
			<div class="column" hostname="TESTING02" executable="RELEASE" branch="bw_2_1_full" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING02" executable="CONSUMER" branch="bw_2_1_full" benchmark="RunProfilerHighlands" name="Consumer"></div>
		</div>
		
		<div class="chart" title="2.1 Urban">
			<div class="column" hostname="TESTING02" executable="RELEASE" branch="bw_2_1_full" benchmark="RunProfilerUrban" name="Release"/></div>
			<div class="column" hostname="TESTING02" executable="CONSUMER" branch="bw_2_1_full" benchmark="RunProfilerUrban" name="Consumer"></div>
		</div>
	</div>
	
	<!--
	<div class="chart-category">
		<h1>2.0 (testing02, full)</h1>
		<div class="chart" title="2.0 Minspec">
			<div class="column" hostname="TESTING02" executable="RELEASE" branch="bw_2_0_full" benchmark="RunProfilerMinspec" name="Release"/></div>
			<div class="column" hostname="TESTING02" executable="CONSUMER" branch="bw_2_0_full" benchmark="RunProfilerMinspec" name="Consumer"></div>
		</div>
		
		<div class="chart" title="2.0 Highlands">
			<div class="column" hostname="TESTING02" executable="RELEASE" branch="bw_2_0_full" benchmark="RunProfilerHighlands" name="Release"/></div>
			<div class="column" hostname="TESTING02" executable="CONSUMER" branch="bw_2_0_full" benchmark="RunProfilerHighlands" name="Consumer"></div>
		</div>
		
		<div class="chart" title="2.0 Urban">
			<div class="column" hostname="TESTING02" executable="RELEASE" branch="bw_2_0_full" benchmark="RunProfilerUrban" name="Release"/></div>
			<div class="column" hostname="TESTING02" executable="CONSUMER" branch="bw_2_0_full" benchmark="RunProfilerUrban" name="Consumer"></div>
		</div>
	</div>
	-->
</body>
</html>