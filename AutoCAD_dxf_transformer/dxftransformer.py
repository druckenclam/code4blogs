import sys
import json

# The implementation is a quick and dirty prototype finished in hours while I am learning
# dxf format 

DBG_VERBOSE = False
DBG_COUNTER = None


class DxfCorruptError(Exception):
    pass

def dxf_assert(assertion, counter):
    if not (assertion):
        raise DxfCorruptError("Corrupt File at counter {0}".format(counter))

def dbg_set_trace_at_counter(counter):
    if DBG_COUNTER and DBG_COUNTER == counter:
        import pudb; pudb.set_trace()     

def dbg_print(*args):
    if DBG_VERBOSE:
        dxf_print(*arg)

def dxf_print(*args):
        counter, code, data = args
        #if start and counter < start: return
        #if end and counter > end: return
        print(
            "{0}: \"{1}\" = \"{2}\"".format(
                counter, code, data))

def read_code_data(dxf_file, counter_list = [-1]):
    counter_list[0] += 1 
    return (counter_list[0], 
            dxf_file.readline().rstrip(), 
            dxf_file.readline().rstrip())

def read_code_data_check(dxf_file):
    counter, code, data = read_code_data(dxf_file)
    dbg_set_trace_at_counter(counter)
    dbg_print(counter, code, data)
    dxf_assert(code != '', counter)
    try:
        numcode = int(code)
    except ValueError:
        dxf_assert(False, counter)
    return (counter, numcode, data)

class Dxf:
    def __init__(self, dxf_file_name):
        self.dxf_file_name = dxf_file_name
        self.comments = ""
        self.sections = []
    def toJSON(self):
        return json.dumps(self, default = lambda o: o.__dict__, indent = 2, sort_keys = True)    
        
class Section:
    def __init__(self, start_counter):
        self._start_counter = start_counter
        self._end_counter = None

        self.type = None
        self.entities = []
    def close(self, end_counter):
        self._end_counter = end_counter

class Entity:
    def __init__(self, start_counter):
        self._start_counter = start_counter
        self._end_counter = None
        # Common Group Code
        self.type = None
        self.handle = None
        self.subclass_maker = None
        self.layer_name = None
        self.linetype_name = None
        self.color_name = None
        self.line_weight = None
        self.points_x = {}
        self.points_y = {}
        self.points_z = {}

    def close(self, end_counter):
        self._end_counter = end_counter

def dxf_process_entities(stack, entity_section, args):
    counter, code, data = args
    if code == 0:
        dxf_assert(isinstance(stack[-1], Section) or
                isinstance(stack[-1], Entity),
                counter)
        entity = Entity(counter)
        entity.type = data
        if isinstance(stack[-1], Entity):
            stack[-1].close(counter - 1)               
            stack.pop()
        stack.append(entity)
        entity_section.entities.append(entity)

    if code == 5:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        stack[-1].handle = data

    if code == 100:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        stack[-1].subclass_maker = data

    if code == 8:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        stack[-1].layer_name = data

    if code == 6:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        stack[-1].linetype_name = data

    if code == 62:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        stack[-1].color_name = data

    if code == 370:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        stack[-1].lineweight = data

    if 10 <= code <= 18:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        try:
            stack[-1].points_x[code] = float(data)
        except ValueError:
            dxf_assert(false, counter)

    if 20 <= code <= 28:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        try:
            stack[-1].points_y[code] = float(data)
        except ValueError:
            dxf_assert(false, counter)

    if 30 <= code <= 38:
        dxf_assert(isinstance(stack[-1], Entity), counter)
        try:
            stack[-1].points_z[code] = float(data)
        except ValueError:
            dxf_assert(false, counter)


def dxf_parse(dxf_file, dxf_file_name, verbose = True):
    stack = [Dxf(dxf_file_name)]
    current_section = None
    while True:
        counter, code, data = read_code_data_check(dxf_file)
        if (code, data) == (0, "EOF"):
            dxf_assert(isinstance(stack[-1], Dxf), counter)
            break

        if (code, data) == (0, "SECTION"):
            section = Section(counter)
            dxf_assert(isinstance(stack[-1], Dxf), counter)
            stack[-1].sections.append(section)
            stack.append(section)
            current_section = section
            continue

        if (code, data) == (0, "ENDSEC"):
            if current_section.type == "ENTITIES":
                # is it possible that ENTITIES section is empty?
                dxf_assert(isinstance(stack[-1], Section) or
                isinstance(stack[-1], Entity),
                counter)
                if isinstance(stack[-1], Entity):
                    stack[-1].close(counter)
                    stack.pop()
            dxf_assert(isinstance(stack[-1], Section), counter) 
            stack[-1].close(counter)
            stack.pop()
            continue

        if (code, data) == (2, "HEADER"):
            dxf_assert(isinstance(stack[-1], Section), counter)
            stack[-1].type = data
            continue

        if (code, data) == (2, "CLASSES"):
            dxf_assert(isinstance(stack[-1], Section), counter)
            stack[-1].type = data
            continue
            
        if (code, data) == (2, "TABLES"):
            dxf_assert(isinstance(stack[-1], Section), counter)
            stack[-1].type = data
            continue

        if (code, data) == (2, "BLOCKS"):
            dxf_assert(isinstance(stack[-1], Section), counter)
            stack[-1].type = data
            continue
        
        if (code, data) == (2, "ENTITIES"):
            dxf_assert(isinstance(stack[-1], Section), counter)
            stack[-1].type = data
            continue

        if (code, data) == (2, "OBJECTS"):
            dxf_assert(isinstance(stack[-1], Section), counter)
            stack[-1].type = data
            continue
        
        if current_section and current_section.type == "ENTITIES":
            dxf_process_entities(stack, current_section, (counter, code, data))

    dxf_json = stack.pop().toJSON()
    if verbose:
        print(dxf_json)
    return dxf_json
       
def dxf_convert(dxf_file, dxf_file_name):
    while True:
        counter, code, data = read_code_data_check(dxf_file)
        dxf_print(counter, code, data)
        if (code, data) == (0, "EOF"):
            break

def dxf_draw(dxf_file, dxf_file_name):
    dxf_json = dxf_parse(dxf_file, dxf_file_name, verbose = False)
    dxf_json_obj = json.loads(dxf_json)
    html_header = '''
    <!DOCTYPE html>
    <html>
        <body>
            <canvas id="myCanvas" width="640" height="480" style="border:5px;">
            Your browser does not support the HTML5 canvas tag.</canvas>
            <script>
                var c = document.getElementById("myCanvas");
                var ctx = c.getContext("2d");
    '''
    html_footer = '''
                ctx.stroke();
            </script>
        </body>
    </html>
    '''
    for section in dxf_json_obj["sections"]:
        if section["type"] != "ENTITIES":
            continue
        print(html_header)
        for entity in section["entities"]:
            if entity["type"] != "LINE":
                continue
            (x0, y0), (x1, y1) = zip(entity["points_x"].values(), 
            entity["points_y"].values())
            #print("{0}, {1} ==> {2}, {3}".format(x0, y0, x1, y1))
            print("ctx.moveTo({0}, {1})".format(int(x0), int(y0)));
            print("ctx.lineTo({0}, {1})".format(int(x1), int(y1)));
        print(html_footer)
        # only one ENTITIES section
        break          
    

def dxf_usage(options, executable_name):
    print("Usage: {0} <dxf_file_name> {1}".format(
        executable_name, "[" + " | ".join(options) + "]")
    )

def main(argv):
    process_table = {
        "txt": dxf_convert,
        "json": dxf_parse,
        "html": dxf_draw
    }            
    try:
        process = process_table[argv[2]]
    except (IndexError, KeyError):
        dxf_usage(process_table.keys(), argv[0])
        sys.exit(-1)
        
    try:
        with open(argv[1]) as file:
            process(file, argv[1])   
    except IndexError:
        dxf_usage(process_table.keys(), argv[0])
    except IOError:
        print("Error: File {0} cannot be opened".format(argv[1]))
    except DxfCorruptError as e:
        print(e)
        raise

if __name__ == "__main__":
    main(sys.argv)
