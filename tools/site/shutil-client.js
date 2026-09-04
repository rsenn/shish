/**
 * Thin JS wrapper around shutil.wasm (see src/sh/sh_util_wasm.c) plus a
 * few AST helpers used by the playground's AST-explorer/rename demo.
 * Plain classic script (not a module) so the site keeps working from a
 * local file:// checkout, where module imports are blocked by CORS.
 *
 * loc fields in the JSON AST are "file:line:col" (1-indexed line/col),
 * and always point one-past-the-end of the token they annotate --
 * this holds for both a bare $name (end lands on the char after the
 * name) and a braced ${name} (end lands exactly on the '}', which is
 * numerically the same "one past the name" position). Assignment
 * strings ("name=value") point at the start of the string instead.
 */
window.ShutilClient = (function () {
  function loadShutil(onStderr, baseUrl) {
    baseUrl = baseUrl || 'assets/';
    return new Promise(function (resolve, reject) {
      var script = document.createElement('script');
      script.src = baseUrl + 'shutil.js';
      script.onerror = function () { reject(new Error('failed to load ' + script.src)); };
      script.onload = function () {
        createShutil({ printErr: onStderr }).then(function (Module) {
          var formatRaw = Module.cwrap('shutil_format', 'number', ['string']);
          var parseRaw = Module.cwrap('shutil_parse_ast', 'number', ['string', 'number']);
          var free = Module.cwrap('shutil_free', null, ['number']);
          var UTF8ToString = Module.UTF8ToString;
          var LOC_MODE = { loc: 0, range: 1, both: 2 };

          resolve({
            /** @returns {{ok: true, text: string} | {ok: false}} */
            format: function (source) {
              var ptr = formatRaw(source);
              if (ptr === 0) return { ok: false };
              var text = UTF8ToString(ptr);
              free(ptr);
              return { ok: true, text: text };
            },
            /**
             * @param {'loc'|'range'|'both'} [locMode] which position field(s)
             *   each node carries; 'range' (or 'both') is what findVarOccurrences
             *   below needs -- 'loc' alone requires no parser support for it.
             * @returns {{ok: true, ast: object} | {ok: false}}
             */
            parse: function (source, locMode) {
              var ptr = parseRaw(source, LOC_MODE[locMode || 'both']);
              if (ptr === 0) return { ok: false };
              var text = UTF8ToString(ptr);
              free(ptr);
              return { ok: true, ast: JSON.parse(text) };
            },
          });
        }, reject);
      };
      document.body.appendChild(script);
    });
  }

  /** Converts one "file:line:col" loc string to a 0-indexed byte offset into `source`. */
  function locToOffset(source, loc) {
    var m = /:(\d+):(\d+)$/.exec(loc);
    if (!m) return -1;
    var line = +m[1], col = +m[2];
    var offset = 0, ln = 1;
    for (var i = 0; i < source.length && ln < line; i++) {
      if (source[i] === '\n') ln++;
      offset = i + 1;
    }
    return offset + (col - 1);
  }

  /** Recursively visits every object with a "kind" field, depth-first. `visit(node, parent, key)`. */
  function walk(node, visit, parent, key) {
    if (Array.isArray(node)) {
      for (var i = 0; i < node.length; i++) walk(node[i], visit, node, i);
      return;
    }
    if (!node || typeof node !== 'object') return;
    if (node.kind) visit(node, parent, key);
    for (var k in node) {
      if (k === 'kind' || k === 'loc') continue;
      walk(node[k], visit, node, k);
    }
  }

  /** Start offset of a node, preferring its "range" field (no line/col
   * math needed) and falling back to locToOffset() on "loc" if a caller
   * only requested loc-mode output. */
  function startOf(node, source) {
    if (Array.isArray(node.range)) return node.range[0];
    if (node.loc) return locToOffset(source, node.loc);
    return -1;
  }

  /**
   * Finds every occurrence of shell variable `name` in `ast` -- each
   * assignment ("name=...") and every $name/${name} parameter expansion
   * referencing it (including inside a ${name:-word} default) -- as a
   * list of {offset, length} spans into `source`, ready for spliceEdits().
   */
  function findVarOccurrences(ast, source, name) {
    var edits = [];

    walk(ast, function (node) {
      if (node.kind === 'word' && Array.isArray(node.list)) {
        var first = node.list[0];
        if (first && first.kind === 'string' && typeof first.stra === 'string' &&
            first.stra.slice(0, name.length + 1) === name + '=') {
          var start = startOf(first, source);
          if (start >= 0) edits.push({ offset: start, length: name.length });
        }
      }

      if (node.kind === 'parameter_expansion' && node.name === name) {
        var pstart = startOf(node, source);
        if (pstart >= 0) edits.push({ offset: pstart, length: name.length });
      }
    });

    return edits;
  }

  /** Applies {offset, length} edits to `source`, highest offset first. */
  function spliceEdits(source, edits, text) {
    var sorted = edits.slice().sort(function (a, b) { return b.offset - a.offset; });
    var out = source;
    for (var i = 0; i < sorted.length; i++) {
      var e = sorted[i];
      out = out.slice(0, e.offset) + text + out.slice(e.offset + e.length);
    }
    return out;
  }

  /** Renames every occurrence of `oldName` to `newName` in `source`, given its parsed `ast`. */
  function renameVar(source, ast, oldName, newName) {
    return spliceEdits(source, findVarOccurrences(ast, source, oldName), newName);
  }

  return {
    loadShutil: loadShutil,
    locToOffset: locToOffset,
    walk: walk,
    findVarOccurrences: findVarOccurrences,
    spliceEdits: spliceEdits,
    renameVar: renameVar,
  };
})();
