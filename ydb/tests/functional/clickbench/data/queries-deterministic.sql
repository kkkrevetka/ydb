/*0*/ SELECT COUNT(*) FROM $data;
/*1*/ SELECT COUNT(*) FROM $data WHERE AdvEngineID <> 0;
/*2*/ SELECT SUM(AdvEngineID), COUNT(*), AVG(ResolutionWidth) FROM $data;
/*3*/ SELECT AVG(UserID) FROM $data;
/*4*/ SELECT COUNT(DISTINCT UserID) FROM $data;
/*5*/ SELECT COUNT(DISTINCT SearchPhrase) FROM $data;
/*6*/ SELECT MIN(EventDate), MAX(EventDate) FROM $data;
/*7*/ SELECT AdvEngineID, COUNT(*) as cnt FROM $data WHERE AdvEngineID <> 0 GROUP BY AdvEngineID ORDER BY cnt DESC;
/*8*/ SELECT RegionID, COUNT(DISTINCT UserID) AS u FROM $data GROUP BY RegionID ORDER BY u DESC LIMIT 10;
/*9*/ SELECT RegionID, SUM(AdvEngineID), COUNT(*) AS c, AVG(ResolutionWidth), COUNT(DISTINCT UserID) FROM $data GROUP BY RegionID ORDER BY c DESC LIMIT 10;
/*10*/ SELECT MobilePhoneModel, COUNT(DISTINCT UserID) AS u FROM $data WHERE MobilePhoneModel <> '' GROUP BY MobilePhoneModel ORDER BY u DESC, MobilePhoneModel DESC LIMIT 10;
/*11*/ SELECT MobilePhone, MobilePhoneModel, COUNT(DISTINCT UserID) AS u FROM $data WHERE MobilePhoneModel <> '' GROUP BY MobilePhone, MobilePhoneModel ORDER BY u DESC, MobilePhone DESC, MobilePhoneModel DESC LIMIT 10;
/*12*/ SELECT SearchPhrase, COUNT(*) AS c FROM $data WHERE SearchPhrase <> '' GROUP BY SearchPhrase ORDER BY c DESC, SearchPhrase LIMIT 10;
/*13*/ SELECT SearchPhrase, COUNT(DISTINCT UserID) AS u FROM $data WHERE SearchPhrase <> '' GROUP BY SearchPhrase ORDER BY u DESC LIMIT 10;
/*14*/ SELECT SearchEngineID, SearchPhrase, COUNT(*) AS c FROM $data WHERE SearchPhrase <> '' GROUP BY SearchEngineID, SearchPhrase ORDER BY c DESC LIMIT 10;
/*15*/ SELECT UserID, COUNT(*) as cnt FROM $data GROUP BY UserID ORDER BY cnt DESC LIMIT 10;
/*16*/ SELECT UserID, SearchPhrase, COUNT(*) as cnt FROM $data GROUP BY UserID, SearchPhrase ORDER BY cnt DESC LIMIT 10;
/*17*/ SELECT UserID, SearchPhrase, COUNT(*) as u FROM $data GROUP BY UserID, SearchPhrase ORDER BY UserID, SearchPhrase, u LIMIT 10;
/*18*/ SELECT UserID, m, SearchPhrase, COUNT(*) as cnt FROM $data GROUP BY UserID, DateTime::GetMinute(EventTime) AS m, SearchPhrase ORDER BY cnt DESC, UserID, m, SearchPhrase LIMIT 10;
/*19*/ SELECT UserID FROM $data WHERE UserID = 435090932899640449;
/*20*/ SELECT COUNT(*) FROM $data WHERE URL LIKE '%google%';
/*21*/ SELECT SearchPhrase, MIN(URL), COUNT(*) AS c FROM $data WHERE URL LIKE '%google%' AND SearchPhrase <> '' GROUP BY SearchPhrase ORDER BY c DESC LIMIT 10;
/*22*/ SELECT SearchPhrase, MIN(URL), MIN(Title), COUNT(*) AS c, COUNT(DISTINCT UserID) FROM $data WHERE Title LIKE '%Google%' AND URL NOT LIKE '%.google.%' AND SearchPhrase <> '' GROUP BY SearchPhrase ORDER BY c DESC, SearchPhrase, column1, column2 LIMIT 10;
/*23*/ SELECT * FROM $data WHERE URL LIKE '%google%' ORDER BY EventTime LIMIT 10;
/*24*/ SELECT SearchPhrase, EventTime FROM $data WHERE SearchPhrase <> '' ORDER BY EventTime LIMIT 10;
/*25*/ SELECT SearchPhrase FROM $data WHERE SearchPhrase <> '' ORDER BY SearchPhrase LIMIT 10;
/*26*/ SELECT SearchPhrase, EventTime FROM $data WHERE SearchPhrase <> '' ORDER BY EventTime, SearchPhrase LIMIT 10;
/*27*/ SELECT CounterID, AVG(length(URL)) AS l, COUNT(*) AS c FROM $data WHERE URL <> '' GROUP BY CounterID HAVING COUNT(*) > 100000 ORDER BY l DESC LIMIT 25;
/*28*/ SELECT key, AVG(length(Referer)) AS l, COUNT(*) AS c, MIN(Referer) FROM $data WHERE Referer <> '' GROUP BY Url::CutWWW(Url::GetHost(Referer)) as key HAVING COUNT(*) > 100000 ORDER BY l DESC LIMIT 25;
/*29*/ SELECT SUM(ResolutionWidth), SUM(ResolutionWidth + 1), SUM(ResolutionWidth + 2), SUM(ResolutionWidth + 3), SUM(ResolutionWidth + 4), SUM(ResolutionWidth + 5), SUM(ResolutionWidth + 6), SUM(ResolutionWidth + 7), SUM(ResolutionWidth + 8), SUM(ResolutionWidth + 9), SUM(ResolutionWidth + 10), SUM(ResolutionWidth + 11), SUM(ResolutionWidth + 12), SUM(ResolutionWidth + 13), SUM(ResolutionWidth + 14), SUM(ResolutionWidth + 15), SUM(ResolutionWidth + 16), SUM(ResolutionWidth + 17), SUM(ResolutionWidth + 18), SUM(ResolutionWidth + 19), SUM(ResolutionWidth + 20), SUM(ResolutionWidth + 21), SUM(ResolutionWidth + 22), SUM(ResolutionWidth + 23), SUM(ResolutionWidth + 24), SUM(ResolutionWidth + 25), SUM(ResolutionWidth + 26), SUM(ResolutionWidth + 27), SUM(ResolutionWidth + 28), SUM(ResolutionWidth + 29), SUM(ResolutionWidth + 30), SUM(ResolutionWidth + 31), SUM(ResolutionWidth + 32), SUM(ResolutionWidth + 33), SUM(ResolutionWidth + 34), SUM(ResolutionWidth + 35), SUM(ResolutionWidth + 36), SUM(ResolutionWidth + 37), SUM(ResolutionWidth + 38), SUM(ResolutionWidth + 39), SUM(ResolutionWidth + 40), SUM(ResolutionWidth + 41), SUM(ResolutionWidth + 42), SUM(ResolutionWidth + 43), SUM(ResolutionWidth + 44), SUM(ResolutionWidth + 45), SUM(ResolutionWidth + 46), SUM(ResolutionWidth + 47), SUM(ResolutionWidth + 48), SUM(ResolutionWidth + 49), SUM(ResolutionWidth + 50), SUM(ResolutionWidth + 51), SUM(ResolutionWidth + 52), SUM(ResolutionWidth + 53), SUM(ResolutionWidth + 54), SUM(ResolutionWidth + 55), SUM(ResolutionWidth + 56), SUM(ResolutionWidth + 57), SUM(ResolutionWidth + 58), SUM(ResolutionWidth + 59), SUM(ResolutionWidth + 60), SUM(ResolutionWidth + 61), SUM(ResolutionWidth + 62), SUM(ResolutionWidth + 63), SUM(ResolutionWidth + 64), SUM(ResolutionWidth + 65), SUM(ResolutionWidth + 66), SUM(ResolutionWidth + 67), SUM(ResolutionWidth + 68), SUM(ResolutionWidth + 69), SUM(ResolutionWidth + 70), SUM(ResolutionWidth + 71), SUM(ResolutionWidth + 72), SUM(ResolutionWidth + 73), SUM(ResolutionWidth + 74), SUM(ResolutionWidth + 75), SUM(ResolutionWidth + 76), SUM(ResolutionWidth + 77), SUM(ResolutionWidth + 78), SUM(ResolutionWidth + 79), SUM(ResolutionWidth + 80), SUM(ResolutionWidth + 81), SUM(ResolutionWidth + 82), SUM(ResolutionWidth + 83), SUM(ResolutionWidth + 84), SUM(ResolutionWidth + 85), SUM(ResolutionWidth + 86), SUM(ResolutionWidth + 87), SUM(ResolutionWidth + 88), SUM(ResolutionWidth + 89) FROM $data;
/*30*/ SELECT SearchEngineID, ClientIP, COUNT(*) AS c, SUM(IsRefresh), AVG(ResolutionWidth) FROM $data WHERE SearchPhrase <> '' GROUP BY SearchEngineID, ClientIP ORDER BY c DESC, SearchEngineID, ClientIP LIMIT 10;
/*31*/ SELECT WatchID, ClientIP, COUNT(*) AS c, SUM(IsRefresh), AVG(ResolutionWidth) FROM $data WHERE SearchPhrase <> '' GROUP BY WatchID, ClientIP ORDER BY c DESC, WatchID, ClientIP LIMIT 10;
/*32*/ SELECT WatchID, ClientIP, COUNT(*) AS c, SUM(IsRefresh), AVG(ResolutionWidth) FROM $data GROUP BY WatchID, ClientIP ORDER BY c DESC, WatchID, ClientIP LIMIT 10;
/*33*/ SELECT URL, COUNT(*) AS c FROM $data GROUP BY URL ORDER BY c DESC LIMIT 10;
/*34*/ SELECT UserID, URL, COUNT(*) AS c FROM $data GROUP BY UserID, URL ORDER BY c DESC, UserID, URL LIMIT 10;
/*35*/ SELECT ClientIP, ClientIP - 1, ClientIP - 2, ClientIP - 3, COUNT(*) AS c FROM $data GROUP BY ClientIP, ClientIP - 1, ClientIP - 2, ClientIP - 3 ORDER BY c DESC, ClientIP LIMIT 10;
/*36*/ SELECT URL, COUNT(*) AS PageViews FROM $data WHERE CounterID = 62 AND EventDate >= Date('2013-07-01') AND EventDate <= Date('2013-07-31') AND DontCountHits = 0 AND IsRefresh = 0 AND URL <> '' GROUP BY URL ORDER BY PageViews DESC LIMIT 10;
/*37*/ SELECT Title, COUNT(*) AS PageViews FROM $data WHERE CounterID = 62 AND EventDate >= Date('2013-07-01') AND EventDate <= Date('2013-07-31') AND DontCountHits = 0 AND IsRefresh = 0 AND Title <> '' GROUP BY Title ORDER BY PageViews DESC LIMIT 10;
/*38*/ SELECT URL, COUNT(*) AS PageViews FROM $data WHERE CounterID = 62 AND EventDate >= Date('2013-07-01') AND EventDate <= Date('2013-07-31') AND IsRefresh = 0 AND IsLink <> 0 AND IsDownload = 0 GROUP BY URL ORDER BY PageViews DESC LIMIT 10 OFFSET 1000;
/*39*/ SELECT TraficSourceID, SearchEngineID, AdvEngineID, Src, Dst, COUNT(*) AS PageViews FROM $data WHERE CounterID = 62 AND EventDate >= Date('2013-07-01') AND EventDate <= Date('2013-07-31') AND IsRefresh = 0 GROUP BY TraficSourceID, SearchEngineID, AdvEngineID, CASE WHEN (SearchEngineID = 0 AND AdvEngineID = 0) THEN Referer ELSE '' END AS Src, URL AS Dst ORDER BY PageViews DESC LIMIT 10 OFFSET 1000;
/*40*/ SELECT URLHash, EventDate, COUNT(*) AS PageViews FROM $data WHERE CounterID = 62 AND EventDate >= Date('2013-07-01') AND EventDate <= Date('2013-07-31') AND IsRefresh = 0 AND TraficSourceID IN (-1, 6) AND RefererHash = 3594120000172545465 GROUP BY URLHash, EventDate ORDER BY PageViews DESC LIMIT 10 OFFSET 100;
/*41*/ SELECT WindowClientWidth, WindowClientHeight, COUNT(*) AS PageViews FROM $data WHERE CounterID = 62 AND EventDate >= Date('2013-07-01') AND EventDate <= Date('2013-07-31') AND IsRefresh = 0 AND DontCountHits = 0 AND URLHash = 2868770270353813622 GROUP BY WindowClientWidth, WindowClientHeight ORDER BY PageViews DESC LIMIT 10 OFFSET 10000;
/*42*/ SELECT Minute, COUNT(*) AS PageViews FROM $data WHERE CounterID = 62 AND EventDate >= Date('2013-07-14') AND EventDate <= Date('2013-07-15') AND IsRefresh = 0 AND DontCountHits = 0 GROUP BY DateTime::ToSeconds(EventTime)/60 As Minute ORDER BY Minute LIMIT 10 OFFSET 1000;