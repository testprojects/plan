SELECT * FROM sections WHERE kk IN (SELECT sk FROM stations WHERE st = '4')
