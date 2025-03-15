#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

struct CvParameter {
    string accession;
    string name;
    string description;
    variant<monostate, int, double, string, vector<double>, map<string, vector<double>>> value;
    shared_ptr<CvParameter> unit;

    CvParameter() = default;
    CvParameter(const string& acc, const string& n, const string& desc = "") 
        : accession(acc), name(n), description(desc) {}
};

struct InputFile {
    string name;
    string location;
    CvParameter fileFormat;
    vector<CvParameter> fileProperties;
};

struct AnalysisSoftware : public CvParameter {
    string version;
    string uri;
};

struct Metadata {
    string label;
    vector<InputFile> inputFiles;
    vector<AnalysisSoftware> analysisSoftware;
    vector<CvParameter> cvParameters;
};

struct QualityMetric : public CvParameter {
    double toDouble() const {
        if (holds_alternative<double>(value)) {
            return get<double>(value);
        } else if (holds_alternative<int>(value)) {
            return static_cast<double>(get<int>(value));
        }
        throw runtime_error("Value is not a number");
    }
};

struct BaseQuality {
    Metadata metadata;
    vector<QualityMetric> qualityMetrics;
};

struct ControlledVocabulary {
    string name;
    string uri;
    string version;
};

struct MzQC {
    string version;
    string creationDate;
    string description;
    string contactName;
    string contactAddress;
    vector<ControlledVocabulary> controlledVocabularies;
    vector<BaseQuality> runQualities;
    vector<BaseQuality> setQualities;
};

class MzQCParser {
public:
    static MzQC fromFile(const string& filepath) {
        ifstream file(filepath);
        if (!file.is_open()) {
            throw runtime_error("Could not open file: " + filepath);
        }

        json j;
        file >> j;

        return fromJson(j);
    }

    static MzQC fromJson(const json& j) {
        if (!j.contains("mzQC")) {
            throw runtime_error("Invalid mzQC file: missing 'mzQC' element");
        }

        const json& mzqcJson = j["mzQC"];
        MzQC mzqc;

        mzqc.version = mzqcJson.value("version", "");
        mzqc.creationDate = mzqcJson.value("creationDate", "");
        mzqc.description = mzqcJson.value("description", "");
        mzqc.contactName = mzqcJson.value("contactName", "");
        mzqc.contactAddress = mzqcJson.value("contactAddress", "");

        if (mzqcJson.contains("controlledVocabularies")) {
            for (const auto& cvJson : mzqcJson["controlledVocabularies"]) {
                ControlledVocabulary cv;
                cv.name = cvJson.value("name", "");
                cv.uri = cvJson.value("uri", "");
                cv.version = cvJson.value("version", "");
                mzqc.controlledVocabularies.push_back(cv);
            }
        }

        if (mzqcJson.contains("runQualities")) {
            for (const auto& rqJson : mzqcJson["runQualities"]) {
                BaseQuality rq;
                parseBaseQuality(rqJson, rq);
                mzqc.runQualities.push_back(rq);
            }
        }

        if (mzqcJson.contains("setQualities")) {
            for (const auto& sqJson : mzqcJson["setQualities"]) {
                BaseQuality sq;
                parseBaseQuality(sqJson, sq);
                mzqc.setQualities.push_back(sq);
            }
        }

        return mzqc;
    }

private:
    static void parseBaseQuality(const json& j, BaseQuality& quality) {
        if (j.contains("metadata")) {
            parseMetadata(j["metadata"], quality.metadata);
        }

        if (j.contains("qualityMetrics")) {
            for (const auto& qmJson : j["qualityMetrics"]) {
                QualityMetric qm;
                parseCvParameter(qmJson, qm);
                quality.qualityMetrics.push_back(qm);
            }
        }
    }

    static void parseMetadata(const json& j, Metadata& metadata) {
        metadata.label = j.value("label", "");

        if (j.contains("inputFiles")) {
            for (const auto& ifJson : j["inputFiles"]) {
                InputFile inputFile;
                inputFile.name = ifJson.value("name", "");
                inputFile.location = ifJson.value("location", "");

                if (ifJson.contains("fileFormat")) {
                    parseCvParameter(ifJson["fileFormat"], inputFile.fileFormat);
                }

                if (ifJson.contains("fileProperties")) {
                    for (const auto& fpJson : ifJson["fileProperties"]) {
                        CvParameter fp;
                        parseCvParameter(fpJson, fp);
                        inputFile.fileProperties.push_back(fp);
                    }
                }

                metadata.inputFiles.push_back(inputFile);
            }
        }

        if (j.contains("analysisSoftware")) {
            for (const auto& asJson : j["analysisSoftware"]) {
                AnalysisSoftware as;
                parseCvParameter(asJson, as);
                as.version = asJson.value("version", "");
                as.uri = asJson.value("uri", "");
                metadata.analysisSoftware.push_back(as);
            }
        }

        if (j.contains("cvParameters")) {
            for (const auto& cvJson : j["cvParameters"]) {
                CvParameter cv;
                parseCvParameter(cvJson, cv);
                metadata.cvParameters.push_back(cv);
            }
        }
    }

    static void parseCvParameter(const json& j, CvParameter& cv) {
        cv.accession = j.value("accession", "");
        cv.name = j.value("name", "");
        cv.description = j.value("description", "");

        if (j.contains("value")) {
            parseValue(j["value"], cv.value);
        }

        if (j.contains("unit")) {
            cv.unit = make_shared<CvParameter>();
            parseCvParameter(j["unit"], *cv.unit);
        }
    }

    static void parseValue(const json& j, variant<monostate, int, double, string, vector<double>, map<string, vector<double>>>& value) {
        if (j.is_number_integer()) {
            value = j.get<int>();
        } else if (j.is_number_float()) {
            value = j.get<double>();
        } else if (j.is_string()) {
            value = j.get<string>();
        } else if (j.is_array()) {
            try {
                value = j.get<vector<double>>();
            } catch (...) {
                cout << "Warning: Could not parse array as vector<double>" << endl;
                value = monostate();
            }
        } else if (j.is_object()) {
            try {
                map<string, vector<double>> tableData;
                for (auto& [key, val] : j.items()) {
                    if (val.is_array()) {
                        tableData[key] = val.get<vector<double>>();
                    }
                }
                if (!tableData.empty()) {
                    value = tableData;
                } else {
                    value = monostate();
                }
            } catch (...) {
                cout << "Warning: Could not parse object as table data" << endl;
                value = monostate();
            }
        } else {
            value = monostate();
        }
    }
};

class MzQCSerializer {
public:
    static json toJson(const MzQC& mzqc) {
        json j;

        j["mzQC"]["version"] = mzqc.version;
        j["mzQC"]["creationDate"] = mzqc.creationDate;
        
        if (!mzqc.description.empty()) {
            j["mzQC"]["description"] = mzqc.description;
        }
        
        if (!mzqc.contactName.empty()) {
            j["mzQC"]["contactName"] = mzqc.contactName;
        }
        
        if (!mzqc.contactAddress.empty()) {
            j["mzQC"]["contactAddress"] = mzqc.contactAddress;
        }

        j["mzQC"]["controlledVocabularies"] = json::array();
        for (const auto& cv : mzqc.controlledVocabularies) {
            json cvJson;
            cvJson["name"] = cv.name;
            cvJson["uri"] = cv.uri;
            if (!cv.version.empty()) {
                cvJson["version"] = cv.version;
            }
            j["mzQC"]["controlledVocabularies"].push_back(cvJson);
        }

        if (!mzqc.runQualities.empty()) {
            j["mzQC"]["runQualities"] = json::array();
            for (const auto& rq : mzqc.runQualities) {
                json rqJson;
                serializeBaseQuality(rq, rqJson);
                j["mzQC"]["runQualities"].push_back(rqJson);
            }
        }

        if (!mzqc.setQualities.empty()) {
            j["mzQC"]["setQualities"] = json::array();
            for (const auto& sq : mzqc.setQualities) {
                json sqJson;
                serializeBaseQuality(sq, sqJson);
                j["mzQC"]["setQualities"].push_back(sqJson);
            }
        }

        return j;
    }

    static string toString(const MzQC& mzqc, bool pretty = false) {
        json j = toJson(mzqc);
        return pretty ? j.dump(2) : j.dump();
    }

    static void toFile(const MzQC& mzqc, const string& filepath, bool pretty = false) {
        ofstream file(filepath);
        if (!file.is_open()) {
            throw runtime_error("Could not open file for writing: " + filepath);
        }

        file << toString(mzqc, pretty);
    }

private:
    static void serializeBaseQuality(const BaseQuality& quality, json& j) {
        j["metadata"] = json::object();
        serializeMetadata(quality.metadata, j["metadata"]);

        j["qualityMetrics"] = json::array();
        for (const auto& qm : quality.qualityMetrics) {
            json qmJson;
            serializeCvParameter(qm, qmJson);
            j["qualityMetrics"].push_back(qmJson);
        }
    }

    static void serializeMetadata(const Metadata& metadata, json& j) {
        if (!metadata.label.empty()) {
            j["label"] = metadata.label;
        }

        j["inputFiles"] = json::array();
        for (const auto& inputFile : metadata.inputFiles) {
            json ifJson;
            ifJson["name"] = inputFile.name;
            ifJson["location"] = inputFile.location;
            
            ifJson["fileFormat"] = json::object();
            serializeCvParameter(inputFile.fileFormat, ifJson["fileFormat"]);

            if (!inputFile.fileProperties.empty()) {
                ifJson["fileProperties"] = json::array();
                for (const auto& fp : inputFile.fileProperties) {
                    json fpJson;
                    serializeCvParameter(fp, fpJson);
                    ifJson["fileProperties"].push_back(fpJson);
                }
            }

            j["inputFiles"].push_back(ifJson);
        }

        j["analysisSoftware"] = json::array();
        for (const auto& as : metadata.analysisSoftware) {
            json asJson;
            serializeCvParameter(as, asJson);
            asJson["version"] = as.version;
            asJson["uri"] = as.uri;
            j["analysisSoftware"].push_back(asJson);
        }

        if (!metadata.cvParameters.empty()) {
            j["cvParameters"] = json::array();
            for (const auto& cv : metadata.cvParameters) {
                json cvJson;
                serializeCvParameter(cv, cvJson);
                j["cvParameters"].push_back(cvJson);
            }
        }
    }

    static void serializeCvParameter(const CvParameter& cv, json& j) {
        j["accession"] = cv.accession;
        j["name"] = cv.name;
        
        if (!cv.description.empty()) {
            j["description"] = cv.description;
        }

        serializeValue(cv.value, j);

        if (cv.unit) {
            j["unit"] = json::object();
            serializeCvParameter(*cv.unit, j["unit"]);
        }
    }

    static void serializeValue(const variant<monostate, int, double, string, vector<double>, map<string, vector<double>>>& value, json& j) {
        if (holds_alternative<monostate>(value)) {
            return;
        } else if (holds_alternative<int>(value)) {
            j["value"] = get<int>(value);
        } else if (holds_alternative<double>(value)) {
            j["value"] = get<double>(value);
        } else if (holds_alternative<string>(value)) {
            j["value"] = get<string>(value);
        } else if (holds_alternative<vector<double>>(value)) {
            j["value"] = get<vector<double>>(value);
        } else if (holds_alternative<map<string, vector<double>>>(value)) {
            j["value"] = json::object();
            for (const auto& [key, vec] : get<map<string, vector<double>>>(value)) {
                j["value"][key] = vec;
            }
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <mzqc_file> [<output_file>]" << endl;
        return 1;
    }

    string inputFile = argv[1];
    
    try {
        cout << "Parsing file: " << inputFile << endl;
        MzQC mzqc = MzQCParser::fromFile(inputFile);
        
        cout << "Successfully parsed mzQC file!" << endl;
        cout << "Version: " << mzqc.version << endl;
        cout << "Creation date: " << mzqc.creationDate << endl;
        cout << "Run qualities: " << mzqc.runQualities.size() << endl;
        cout << "Set qualities: " << mzqc.setQualities.size() << endl;
        cout << "Controlled vocabularies: " << mzqc.controlledVocabularies.size() << endl;
        
        if (!mzqc.runQualities.empty()) {
            cout << "\nRun Qualities:" << endl;
            for (size_t i = 0; i < mzqc.runQualities.size(); ++i) {
                const auto& rq = mzqc.runQualities[i];
                string label = rq.metadata.label.empty() ? "Run " + to_string(i+1) : rq.metadata.label;
                cout << "  " << label << ": " << rq.qualityMetrics.size() << " metrics" << endl;
                
                for (const auto& qm : rq.qualityMetrics) {
                    cout << "    - " << qm.name << " (" << qm.accession << ")";
                    
                    if (holds_alternative<int>(qm.value)) {
                        cout << ": " << get<int>(qm.value);
                    } else if (holds_alternative<double>(qm.value)) {
                        cout << ": " << get<double>(qm.value);
                    } else if (holds_alternative<string>(qm.value)) {
                        cout << ": \"" << get<string>(qm.value) << "\"";
                    } else if (holds_alternative<vector<double>>(qm.value)) {
                        cout << ": [";
                        const auto& vec = get<vector<double>>(qm.value);
                        for (size_t j = 0; j < vec.size(); ++j) {
                            if (j > 0) cout << ", ";
                            cout << vec[j];
                            if (j >= 2 && vec.size() > 4) {
                                cout << ", ...";
                                break;
                            }
                        }
                        cout << "]";
                    } else if (holds_alternative<map<string, vector<double>>>(qm.value)) {
                        cout << ": Table data";
                    }
                    
                    if (qm.unit) {
                        cout << " " << qm.unit->name;
                    }
                    
                    cout << endl;
                }
            }
        }
        
        if (argc > 2) {
            string outputFile = argv[2];
            cout << "\nWriting to file: " << outputFile << endl;
            MzQCSerializer::toFile(mzqc, outputFile, true);
            cout << "File written successfully!" << endl;
        }
        
        return 0;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}