
var shell = new ActiveXObject('WScript.Shell')
var fso = new ActiveXObject( "Scripting.FileSystemObject" )
var sql = new ActiveXObject('ADODB.Connection')
sql = Session.Connection;
WIDTH = 21
HEADERS = '#IdDk;DataDk;MiejsceDk;TerminDk;NumerDk;IdKh;NazwaKh;KodPocztKh;MiastoKh;UlicaKh;DomKh;MieszkanieKh;NIPKh;KodKh;OpisDk;IdPoz;IdTw;NazwaTw;KodTw;IloscTw;JmTw'
OUTPUT_CSV = ??folder
print(OUTPUT_CSV)
main()

// ************************* TWORZENIE PLIKU *************************
function main() {
	var id_arr = []
	while(!Recordset.EOF) {
		id_arr.push(Recordset.Fields(0).Value)
		Recordset.moveNext()
	}
	serialized_data = getSerializedData(id_arr)
	print(1)
	createAndWrite(OUTPUT_CSV, serialized_data)
	print(1)
}

// ************************* POBRANIE DANYCH *************************
function select(query) {
	return sql.Execute(query)
}

function getSerializedData(id_arr) {
	var id_string = stringify(id_arr)
	var query = "SELECT * \
				FROM   (SELECT trn_trnid, \
					   CONVERT(CHAR(10), trn_datadok, 126) AS data_dok, \
					   trn_podmiasto, \
					   CONVERT(CHAR(10), trn_termin, 126)  AS termin, \
					   trn_numerpelny, \
					   knt_kntid, \
					   knt_nazwa1, \
					   replace(knt_kodpocztowy, '-', '') as kod_pocztowy, \
					   knt_miasto, \
					   knt_ulica, \
					   knt_nrdomu, \
					   knt_nrlokalu, \
					   knt_nip, \
					   knt_kod, \
					   REPLACE(trn_opis, ';', ',') as opis \
				FROM   cdn.tranag \
					   JOIN cdn.podmiotyview \
						 ON trn_podid = pod_podid \
					   JOIN cdn.kontrahenci \
						 ON pod_kod = knt_kod \
				WHERE  trn_trnid IN (" + id_string + ")) TRANAG \
				JOIN (SELECT tre_treid, \
							tre_twrid, \
							twr_nazwa, \
							twr_kod, \
							tre_ilosc, \
							tre_jm, \
							tre_trnid \
					 FROM   cdn.traelem \
							JOIN cdn.towary \
							  ON tre_twrid = twr_twrid \
					 WHERE  tre_trnid IN (" + id_string + ")) TRAELEM \
				ON TRANAG.trn_trnid = TRAELEM.tre_trnid \
				ORDER  BY 	trn_trnid ASC, \
							tre_treid ASC"
				
	var dataRS = select(query)
	var serialized_data = ''
	while(!dataRS.EOF) {
		for(var i = 0; i < WIDTH; i++) {
			serialized_data += dataRS.Fields(i).Value + ';'
		}
		serialized_data += '\n'
		dataRS.MoveNext()
	}
	return serialized_data
}

function stringify(arr) {
	var res = ''
	for(var i = 0; i < arr.length; i++) {
		res += arr[i] + ', '
	}
	return res.substr(0, res.length - 2)
}
	
// ************************* NARZEDZIA *************************

function createAndWrite(filename, content) {
	var file = fso.CreateTextFile(filename, true)
	file.WriteLine(HEADERS)
	file.WriteLine(content)
	file.Close()
}

function serialize(arr) {
	var text = ''
	for(var i = 0; i < arr.length; i++) {
		text += arr[i] + ';'
	}
	return text
}

function eachArr(list, fun) {
	var result = []
	for(var i = 0; i < list.length; i++) {
		result.push(fun(list[i]))
	}
	return result
}


function printArr (arr) {
	var message = ''
	for(var i = 0; i < arr.length; i++) {
		message += arr[i] + ';'
	}
	print(message)
}


function print (text) {
	shell.popup(text)
}