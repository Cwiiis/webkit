// Copyright (C) 2015 Mike Pennisi. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.
/*---
esid: sec-array.prototype-@@unscopables
description: >
    Initial value of `Symbol.unscopables` property
info: |
    22.1.3.32 Array.prototype [ @@unscopables ]

    1. Let unscopableList be ObjectCreate(null).
    2. Perform CreateDataProperty(unscopableList, "copyWithin", true).
    3. Perform CreateDataProperty(unscopableList, "entries", true).
    4. Perform CreateDataProperty(unscopableList, "fill", true).
    5. Perform CreateDataProperty(unscopableList, "find", true).
    6. Perform CreateDataProperty(unscopableList, "findIndex", true).
    7. Perform CreateDataProperty(unscopableList, "findLast", true).
    8. Perform CreateDataProperty(unscopableList, "findLastIndex", true).
    9. Perform CreateDataProperty(unscopableList, "flat", true).
    10. Perform CreateDataProperty(unscopableList, "flatMap", true).
    11. Perform ! CreateDataPropertyOrThrow(unscopableList, "groupBy", true).
    12. Perform ! CreateDataPropertyOrThrow(unscopableList, "groupByToMap", true).
    13. Perform CreateDataProperty(unscopableList, "includes", true).
    14. Perform CreateDataProperty(unscopableList, "keys", true).
    15. Perform CreateDataProperty(unscopableList, "values", true).
    16. Assert: Each of the above calls returns true.
    17. Return unscopableList.
includes: [propertyHelper.js]
features: [Symbol.unscopables, array-find-from-last]
---*/

var unscopables = Array.prototype[Symbol.unscopables];

assert.sameValue(Object.getPrototypeOf(unscopables), null);

assert.sameValue(unscopables.copyWithin, true, '`copyWithin` property value');
verifyEnumerable(unscopables, 'copyWithin');
verifyWritable(unscopables, 'copyWithin');
verifyConfigurable(unscopables, 'copyWithin');

assert.sameValue(unscopables.entries, true, '`entries` property value');
verifyEnumerable(unscopables, 'entries');
verifyWritable(unscopables, 'entries');
verifyConfigurable(unscopables, 'entries');

assert.sameValue(unscopables.fill, true, '`fill` property value');
verifyEnumerable(unscopables, 'fill');
verifyWritable(unscopables, 'fill');
verifyConfigurable(unscopables, 'fill');

assert.sameValue(unscopables.find, true, '`find` property value');
verifyEnumerable(unscopables, 'find');
verifyWritable(unscopables, 'find');
verifyConfigurable(unscopables, 'find');

assert.sameValue(unscopables.findIndex, true, '`findIndex` property value');
verifyEnumerable(unscopables, 'findIndex');
verifyWritable(unscopables, 'findIndex');
verifyConfigurable(unscopables, 'findIndex');


assert.sameValue(unscopables.findLast, true, '`findLast` property value');
verifyEnumerable(unscopables, 'findLast');
verifyWritable(unscopables, 'findLast');
verifyConfigurable(unscopables, 'findLast');

assert.sameValue(unscopables.findLastIndex, true, '`findLastIndex` property value');
verifyEnumerable(unscopables, 'findLastIndex');
verifyWritable(unscopables, 'findLastIndex');
verifyConfigurable(unscopables, 'findLastIndex');

assert.sameValue(unscopables.flat, true, '`flat` property value');
verifyEnumerable(unscopables, 'flat');
verifyWritable(unscopables, 'flat');
verifyConfigurable(unscopables, 'flat');

assert.sameValue(unscopables.flatMap, true, '`flatMap` property value');
verifyEnumerable(unscopables, 'flatMap');
verifyWritable(unscopables, 'flatMap');
verifyConfigurable(unscopables, 'flatMap');

assert.sameValue(unscopables.groupBy, true, '`groupBy` property value');
verifyEnumerable(unscopables, 'groupBy');
verifyWritable(unscopables, 'groupBy');
verifyConfigurable(unscopables, 'groupBy');

assert.sameValue(unscopables.groupByToMap, true, '`groupByToMap` property value');
verifyEnumerable(unscopables, 'groupByToMap');
verifyWritable(unscopables, 'groupByToMap');
verifyConfigurable(unscopables, 'groupByToMap');

assert.sameValue(unscopables.includes, true, '`includes` property value');
verifyEnumerable(unscopables, 'includes');
verifyWritable(unscopables, 'includes');
verifyConfigurable(unscopables, 'includes');

assert.sameValue(unscopables.keys, true, '`keys` property value');
verifyEnumerable(unscopables, 'keys');
verifyWritable(unscopables, 'keys');
verifyConfigurable(unscopables, 'keys');

assert.sameValue(unscopables.values, true, '`values` property value');
verifyEnumerable(unscopables, 'values');
verifyWritable(unscopables, 'values');
verifyConfigurable(unscopables, 'values');
