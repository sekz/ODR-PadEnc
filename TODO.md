# ODR-PadEnc TODO List

**Last Updated:** October 19, 2025  
**Status:** Planning Phase

---

## 🎯 Priority Tasks

### 1. DL Plus (Dynamic Label Plus) Implementation - TS 102 980

**Priority:** 🟡 MEDIUM (Optional but High Value)  
**Timeline:** 3-4 weeks  
**Standard:** ETSI TS 102 980 V2.1.1

#### 1.1 DL Plus Encoder Development

**Objective:** Implement DL Plus encoder to provide structured metadata for DAB+ services.

**Tasks:**
- [ ] Research ETSI TS 102 980 V2.1.1 specification
  - Study DL Plus object structure
  - Understand tag encoding format
  - Review content type codes
  - Analyze X-PAD integration
  
- [ ] Design DL Plus encoder architecture
  - Choose implementation approach:
    - Option A: Integrate into odr-padenc directly (C++)
    - Option B: External tool with API integration (Python/C++)
    - Option C: Library-based approach (reusable component)
  - Design tag management system
  - Plan X-PAD data embedding
  
- [ ] Implement core DL Plus functionality
  - [ ] DL Plus object encoder
  - [ ] Tag type support (see section 1.2)
  - [ ] Character set handling (UTF-8, EBU Latin)
  - [ ] Tag length validation (max 128 bytes per tag)
  - [ ] CRC calculation for DL Plus objects
  
- [ ] X-PAD Integration
  - [ ] Embed DL Plus in X-PAD data group
  - [ ] Respect X-PAD capacity limits
  - [ ] Handle MOT and DL Plus coexistence
  - [ ] Tag rate limiting (max 1 tag/second recommended)

#### 1.2 Supported Tag Types

**Must Have (Priority 1):**
- [ ] **ITEM.TITLE** (Content Item Title)
  - Thai: ชื่อเพลง / ชื่อรายการ
  - Examples: เพลง "รักแท้", รายการ "ข่าวเช้า"
  
- [ ] **ITEM.ARTIST** (Content Item Artist)
  - Thai: ศิลปิน / ผู้ดำเนินรายการ
  - Examples: "เบิร์ด ธงไชย", "พิธีกรโอ๊ต"
  
- [ ] **STATIONNAME.LONG** (Station Long Name)
  - Thai: ชื่อสถานีวิทยุแบบยาว
  - Examples: "สถานีวิทยุไทยพีบีเอส", "Cool Fahrenheit 93"

**Nice to Have (Priority 2):**
- [ ] **ITEM.ALBUM** (Album Name)
  - Thai: ชื่ออัลบั้ม
  
- [ ] **INFO.NEWS** (News Information)
  - Thai: ข่าวสาร
  - Use for news headlines
  
- [ ] **INFO.WEATHER** (Weather Information)
  - Thai: สภาพอากาศ
  - Temperature, conditions, forecasts
  
- [ ] **PROGRAMME.NOW** (Current Programme)
  - Thai: รายการปัจจุบัน
  
- [ ] **PROGRAMME.NEXT** (Next Programme)
  - Thai: รายการถัดไป

**Advanced (Priority 3):**
- [ ] **INFO.SPORT** (Sports Information)
- [ ] **INFO.LOTTERY** (Lottery Results)
- [ ] **PHONE.HOTLINE** (Contact Number)
- [ ] **PHONE.STUDIO** (Studio Number)
- [ ] **DESCRIPTOR.PLACE** (Location)
- [ ] **DESCRIPTOR.EVENT** (Event Information)

#### 1.3 Thai Language Support

- [ ] **UTF-8 Encoding**
  - Thai characters (U+0E00 to U+0E7F)
  - Tone marks and diacritics
  - Zero-width characters handling
  - String length validation (visual vs. byte length)
  
- [ ] **Character Set Selection**
  - UTF-8 (preferred for Thai)
  - EBU Latin (for compatibility)
  - Automatic encoding detection
  - Fallback mechanisms
  
- [ ] **Input Validation**
  - Thai character validation
  - Profanity filtering (optional)
  - Length constraints per tag type
  - Special character handling

#### 1.4 API Integration

**RESTful API Endpoints:**

- [ ] **POST /api/v1/dlplus/tag**
  - Create and send DL Plus tag
  - Request body:
    ```json
    {
      "tag_type": "ITEM.TITLE",
      "content": "รักแท้",
      "charset": "utf-8"
    }
    ```
  - Response: Tag sent confirmation
  
- [ ] **POST /api/v1/dlplus/batch**
  - Send multiple tags at once
  - Example: Title + Artist + Album
  
- [ ] **GET /api/v1/dlplus/templates**
  - List available tag templates
  
- [ ] **POST /api/v1/dlplus/template**
  - Apply pre-defined tag template
  
- [ ] **GET /api/v1/dlplus/history**
  - Retrieve tag history (last 100 tags)
  
- [ ] **GET /api/v1/dlplus/status**
  - Current DL Plus encoder status

**WebSocket Support:**
- [ ] Real-time tag updates
- [ ] Tag preview before broadcast
- [ ] Tag confirmation events

#### 1.5 GUI Integration Requirements

**For EncoderManager Frontend:**
- [ ] DL Plus configuration page
  - Enable/disable DL Plus per encoder instance
  - Tag template management
  - Manual tag input form
  - Real-time tag preview
  
- [ ] Tag Templates
  - Create/edit/delete templates
  - Template categories (Music, Talk Show, News, etc.)
  - Thai language presets
  
- [ ] Tag History
  - Display last 50 tags sent
  - Filter by tag type
  - Export to CSV/JSON
  
- [ ] Real-time Preview
  - Show how tags will appear on receivers
  - Character count indicator
  - Validation warnings

**For StreamDAB-Mux Frontend:**
- [ ] Service-level DL Plus settings
  - Enable/disable DL Plus per DAB service
  - Tag rate configuration (tags/second)
  - Character set selection
  - FIG 0/13 signaling validation

#### 1.6 Testing & Validation

- [ ] **Unit Tests**
  - Tag encoding correctness
  - Character set conversion
  - CRC calculation
  - X-PAD embedding
  
- [ ] **Integration Tests**
  - ODR-PadEnc → ODR-DabMux pipeline
  - EncoderManager API → ODR-PadEnc
  - End-to-end tag delivery
  
- [ ] **Receiver Tests**
  - Test with compatible DAB+ receivers
  - Validate Thai character rendering
  - Test all tag types
  - Long-term stability testing
  
- [ ] **Performance Tests**
  - Tag throughput (tags/second)
  - Latency measurement
  - Memory usage
  - CPU usage

#### 1.7 Documentation

- [ ] **Developer Documentation**
  - API reference (OpenAPI/Swagger)
  - Code examples (Python, curl, JavaScript)
  - Architecture diagrams
  
- [ ] **User Guide**
  - How to configure DL Plus
  - Tag template creation guide
  - Troubleshooting common issues
  - Thai language best practices
  
- [ ] **Compliance Documentation**
  - ETSI TS 102 980 compliance checklist
  - Test results and validation
  - Receiver compatibility matrix

---

## 📚 Technical References

### ETSI Standards
- **TS 102 980 V2.1.1** - Dynamic Label Plus (DL Plus)
  - https://www.etsi.org/deliver/etsi_ts/102900_102999/102980/02.01.01_60/
  
- **EN 300 401 V2.1.1** - DAB System Specification
  - Section 7.4: X-PAD
  - Section 8.1: FIG 0/13 (User Application Information)

### Related Documents
- `/docs/compliance/MANDATORY-COMPLIANCE-GAP-ANALYSIS.md` - Gap analysis
- `/docs/compliance/THAI-DAB-COMPLIANCE-REPORT.md` - Compliance report
- `/StreamDAB-Mux/ODR-DabMux/THAI-DAB-IMPLEMENTATION-STATUS.md` - Mux status

---

## 🔧 Implementation Options

### Option A: Integrated C++ Module (Recommended)
**Pros:**
- ✅ Native performance
- ✅ Direct X-PAD access
- ✅ No external dependencies
- ✅ Lower latency

**Cons:**
- ❌ Requires C++ expertise
- ❌ Longer development time
- ❌ Harder to maintain

### Option B: External Python Tool
**Pros:**
- ✅ Faster development
- ✅ Easier to prototype
- ✅ Better for complex logic
- ✅ Easier Thai string handling

**Cons:**
- ❌ API overhead
- ❌ Additional process to manage
- ❌ Potential latency issues

### Option C: Hybrid Approach
**Pros:**
- ✅ C++ core encoder
- ✅ Python API wrapper
- ✅ Best of both worlds

**Cons:**
- ❌ More complex architecture
- ❌ Two codebases to maintain

**Recommendation:** Start with **Option B** (Python tool) for rapid prototyping, then migrate to **Option A** (C++ module) if performance is critical.

---

## 📅 Proposed Timeline

### Week 1: Research & Design
- [ ] Study ETSI TS 102 980 specification
- [ ] Design architecture (choose option A/B/C)
- [ ] Create detailed implementation plan
- [ ] Set up development environment

### Week 2: Core Implementation
- [ ] Implement DL Plus object encoder
- [ ] Support Priority 1 tags (TITLE, ARTIST, STATIONNAME)
- [ ] Thai UTF-8 encoding support
- [ ] Basic X-PAD embedding

### Week 3: API & Integration
- [ ] REST API endpoints
- [ ] ODR-DabMux integration
- [ ] Tag rate limiting
- [ ] Error handling

### Week 4: Testing & Documentation
- [ ] Unit and integration tests
- [ ] Receiver compatibility testing
- [ ] API documentation
- [ ] User guide

---

## 🎬 Quick Start (When Ready)

```bash
# 1. Install dependencies (if using Python tool)
pip install -r requirements-dlplus.txt

# 2. Configure DL Plus
./odr-padenc --enable-dlplus \
             --dlplus-api-port 9100 \
             --charset utf-8

# 3. Send a test tag (via API)
curl -X POST http://localhost:9100/api/v1/dlplus/tag \
  -H "Content-Type: application/json" \
  -d '{
    "tag_type": "ITEM.TITLE",
    "content": "รักแท้",
    "charset": "utf-8"
  }'

# 4. Verify in logs
tail -f /var/log/odr-padenc/dlplus.log
```

---

## 📊 Success Metrics

- [ ] All Priority 1 tags working correctly
- [ ] Thai characters display correctly on receivers
- [ ] Tag latency < 2 seconds (from API call to broadcast)
- [ ] Zero crashes over 72-hour stability test
- [ ] API response time < 100ms
- [ ] Successfully tested with ≥3 different DAB+ receivers
- [ ] Documentation complete and reviewed

---

## 🚀 Future Enhancements (Post-MVP)

- [ ] Dynamic tag scheduling (future tags)
- [ ] Tag A/B testing support
- [ ] Analytics dashboard (tag usage statistics)
- [ ] Machine learning tag suggestions
- [ ] Integration with music databases (Spotify, Apple Music)
- [ ] Automatic tag generation from audio metadata
- [ ] Multi-language tag support (Thai, English, Chinese)

---

**Status:** 📋 Planning - Ready for development approval  
**Blocked By:** None  
**Next Step:** Choose implementation option (A/B/C) and begin Week 1 research
